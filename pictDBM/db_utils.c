/* ** NOTE: undocumented in Doxygen
 * @file db_utils.c
 * @brief implementation of several tool functions for pictDB
 *
 * @author Mia Primorac
 * @date 2 Nov 2015
 *
 * Last modification :
 * @author Alexis Montavon and Dorian Laforest
 */

#include "pictDB.h"

/********************************************************************//**
 * Human-readable SHA
 */
static void sha_to_string(const unsigned char* SHA, char* sha_string)
{
    if(SHA == NULL) {
        return;
    }
    for(size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i*2], "%02x", SHA[i]);
    }

    sha_string[2*SHA256_DIGEST_LENGTH] = '\0';
}

/********************************************************************//**
 * pictDB header display.
 */
void print_header(const struct pictdb_header* header)
{
    printf("*****************************************\n");
    printf("**********DATABASE HEADER START**********\n");
    printf("DB NAME: %31s\n", header->db_name);
    printf("VERSION: %" PRIu32 "\n", header->db_version);
    printf("IMAGE COUNT: %" PRIu32 "\t\tMAX IMAGES: %" PRIu32 "\n", header->num_files, header->max_files);
    printf("THUMBNAIL: %" PRIu16 " x %" PRIu16 "\tSMALL: %" PRIu16 " x %" PRIu16 "\n", header->res_resized[DIM_X_THUMB],
           header->res_resized[DIM_Y_THUMB], header->res_resized[DIM_X_SMALL], header->res_resized[DIM_Y_SMALL]);
    printf("***********DATABASE HEADER END***********\n");
    printf("*****************************************\n");
}

/********************************************************************//**
 * Metadata display.
 */
void print_metadata(const struct pict_metadata* metadata)
{
    char sha_printable[2*SHA256_DIGEST_LENGTH+1];
    sha_to_string(metadata->SHA, sha_printable);

    printf("PICTURE ID: %s\n", metadata->pict_id);
    printf("SHA: %s\n", sha_printable);
    printf("VALID: %" PRIu16 "\n", metadata->is_valid);
    printf("UNUSED: %" PRIu16 "\n", metadata->unused_16);
    printf("OFFSET ORIG. : %" PRIu64 "\t\tSIZE ORIG. : %" PRIu32 "\n", metadata->offset[RES_ORIG], metadata->size[RES_ORIG]);
    printf("OFFSET THUMB.: %" PRIu64 "\t\tSIZE THUMB.: %" PRIu32 "\n", metadata->offset[RES_THUMB], metadata->size[RES_THUMB]);
    printf("OFFSET SMALL : %" PRIu64 "\t\tSIZE SMALL : %" PRIu32 "\n", metadata->offset[RES_SMALL], metadata->size[RES_SMALL]);
    printf("ORIGINAL: %" PRIu32 " x %" PRIu32 "\n", metadata->res_orig[DIM_X_ORIG], metadata->res_orig[DIM_Y_ORIG]);
    printf("*****************************************\n");
}

/********************************************************************//**
 * Open the database file and load its content in memory.
 */
int do_open(const char* db_filename, const char* open_mode, struct pictdb_file* db_file)
{
    //Test if the pointer are not NULL
    //In this project, pictDBM, we made the decision to return the error
    //ERR_INVALID_ARGUMENT because we did not found a better one and
    //because this error should not occur since the argument are already
    //tested before the call of this function. But if this function is used
    //in an other library, the argument should be tested
    if((NULL == db_filename) || (NULL == open_mode) || (NULL == db_file)) {
        return ERR_INVALID_ARGUMENT;
    }

    //Test filename
    if(strlen(db_filename) > FILENAME_MAX && strlen(db_filename) != 0) {
        return ERR_INVALID_FILENAME;
    }

    //Opening the file in the specify mode
    db_file->fpdb = fopen(db_filename, open_mode);
    if(db_file->fpdb == NULL) {
        switch(errno) {
        //mode argument invalid
        case EINVAL:
            return ERR_INVALID_ARGUMENT;
            break;
        //wrong path/filename
        case ENOENT:
            return ERR_FILE_NOT_FOUND;
            break;
        default:
            return ERR_IO;
            break;
        }
    }

    //Read and load header
    if(fread(&db_file->header, sizeof(struct pictdb_header), 1, db_file->fpdb) != 1) {
        do_close(db_file);
        return ERR_IO;
    }
    //Test if the data are valid
    else {
        if(db_file->header.num_files > MAX_MAX_FILES) {
            do_close(db_file);
            return ERR_MAX_FILES;
        }
    }

    //Allocating dynamiclly the metadata
    db_file->metadata = calloc(db_file->header.max_files, sizeof(struct pict_metadata));
    if(db_file->metadata == NULL) {
        do_close(db_file);
        return ERR_OUT_OF_MEMORY;
    }

    //Read and load metadata
    if(fread(db_file->metadata, sizeof(struct pict_metadata), db_file->header.max_files, db_file->fpdb) != db_file->header.max_files) {
        do_close(db_file);
        return ERR_IO;
    }

    return 0;
}

/********************************************************************//**
 * Close the database's file and free the metadatas pointer
 */
void do_close(struct pictdb_file* db_file)
{
    if(db_file->fpdb != NULL) {
        fclose(db_file->fpdb);
        db_file->fpdb = NULL;
    }
    if(db_file->metadata != NULL) {
        free(db_file->metadata);
        db_file->metadata = NULL;
    }
}

/********************************************************************//**
 * Converts a string into the wanted resolution.
 */
int resolution_atoi(const char* resolution)
{
    if(resolution != NULL) {
        if(strcmp(resolution, "thumb") == 0 || strcmp(resolution, "thumbnail") == 0) {
            return RES_THUMB;
        } else if(strcmp(resolution, "small") == 0) {
            return RES_SMALL;
        } else if(strcmp(resolution, "orig") == 0 || strcmp(resolution, "original") == 0) {
            return RES_ORIG;
        }
    }
    return -1;
}

/********************************************************************//**
 * Read an image from file.
 */
int read_disk_image(char** image_buffer, const uint32_t image_size, FILE* f)
{
    *image_buffer = calloc(image_size, sizeof(char));
    if(NULL == *image_buffer) {
        return ERR_OUT_OF_MEMORY;
    }
    if(1 != fread(*image_buffer, image_size, 1, f)) {
        free(*image_buffer);
        image_buffer = NULL;
        return ERR_IO;
    }
    return 0;
}

/********************************************************************//**
 * Read an image from a db_file.
 */
int read_db_file_image(char** image_buffer, const size_t index, const size_t dim, const struct pictdb_file* db_file)
{
    if(0 != fseek(db_file->fpdb, db_file->metadata[index].offset[dim], SEEK_SET)) {
        return ERR_IO;
    }
    size_t size = db_file->metadata[index].size[dim];
    return read_disk_image(image_buffer, size, db_file->fpdb);
}

/********************************************************************//**
 * Write an image on the disk
 */
int write_disk_image(const char* image_buffer, const uint32_t image_size, FILE* f)
{
    if(1 != fwrite(image_buffer, image_size, 1, f)) {
        return ERR_IO;
    }
    return 0;
}

/********************************************************************//**
 * Write an image at the end of a db_file and store its offset in offset.
 */
int write_db_file_image(const char* image_buffer, const uint32_t image_size, long* offset, struct pictdb_file* db_file)
{
    if(0 != fseek(db_file->fpdb, 0L, SEEK_END)) {
        return ERR_IO;
    }
    *offset = ftell(db_file->fpdb);
    if(-1 == *offset) {
        return ERR_IO;
    }
    return write_disk_image(image_buffer, image_size, db_file->fpdb);
}


/********************************************************************//**
 * Get the size of the image in the file.
 */
int get_image_size(FILE* image, long* size)
{
    if(fseek(image, 0L, SEEK_END) != 0) {
        return  ERR_IO;
    } else {
        *size = ftell(image);
        if(*size == -1) {
            return ERR_IO;
        } else {
            if(fseek(image, 0L, SEEK_SET) != 0) {
                return ERR_IO;
            }
            return 0;
        }
    }
}

/********************************************************************//**
 * Get the picture's index corresponding to pictID.
 */
int get_image_index(const char* pictID, size_t* index, const struct pictdb_file* db_file)
{
    *index = 0;
    int found = 0;
    while((*index < db_file->header.max_files) && (0 == found)) {
        if((NON_EMPTY == db_file->metadata[*index].is_valid) && (0 == strcmp(db_file->metadata[*index].pict_id, pictID))) {
            found = 1;
        }
        ++(*index);
    }
    if(0 == found) {
        return ERR_FILE_NOT_FOUND;
    }
    //If found = 1, the index was the previous value of i
    --(*index);
    return 0;
}

/********************************************************************//**
 * Write the header on the db_file.
 */
int write_db_file_header(const struct pictdb_file* db_file)
{
    if(0 != fseek(db_file->fpdb, 0L, SEEK_SET)) {
        return ERR_IO;
    }
    if(1 != fwrite(&db_file->header, sizeof(struct pictdb_header), 1, db_file->fpdb)) {
        return ERR_IO;
    }
    return 0;
}

/********************************************************************//**
 * Write a metadata on the db_file.
 */
int write_db_file_one_metadata(const struct pictdb_file* db_file, size_t index)
{
    if(0 != fseek(db_file->fpdb, sizeof(struct pictdb_header) + index * sizeof(struct pict_metadata), SEEK_SET)) {
        return ERR_IO;
    }
    if(1 != fwrite(&db_file->metadata[index], sizeof(struct pict_metadata), 1, db_file->fpdb)) {
        return ERR_IO;
    }
    return 0;
}

/********************************************************************//**
 * Compare two SHAs
 */
int cmp_SHA(const unsigned char* SHA_1, const unsigned char* SHA_2)
{
    if(SHA_1 == NULL || SHA_2 == NULL) {
        return ERR_INVALID_ARGUMENT;
    } else {
        return memcmp(SHA_1, SHA_2, SHA256_DIGEST_LENGTH);
    }
}
