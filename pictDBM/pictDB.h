/**
 * @file pictDB.h
 * @brief Main header file for pictDB core library.
 *
 * Defines the format of the data structures that will be stored on the disk
 * and provides interface functions.
 *
 * The picture database starts with exactly one header structure
 * followed by exactly pictdb_header.max_files metadata
 * structures. The actual content is not defined by these structures
 * because it should be stored as raw bytes appended at the end of the
 * database file and addressed by offsets in the metadata structure.
 *
 * @author Mia Primorac
 * @date 2 Nov 2015
 *
 * Last modification :
 * @author Alexis Montavon and Dorian Laforest
 */

#ifndef PICTDBPRJ_PICTDB_H
#define PICTDBPRJ_PICTDB_H

#include "error.h" /* not needed here, but provides it as required by
                    * all functions of this lib.
                    */
#include <stdio.h> // for FILE
#include <stdint.h> // for uint32_t, uint64_t
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH
#include <string.h> // strncpy and strcmp
#include <errno.h> // for errno
#include <inttypes.h> // for PRIuX
#include <stdlib.h>

#define CAT_TXT "EPFL PictDB binary"

/* constraints */
#define MAX_DB_NAME 31  // Max. size of a PictDB name
#define MAX_PIC_ID 127  // Max. size of a picture id
#define DEFAULT_MAX_FILES 10 // Default value of max_files
#define MAX_MAX_FILES 100000 // Max. value of max_files
#define DEFAULT_THUMB 64 // Default thumbnail size
#define MAX_THUMB 128 // Max. thumbnail size
#define DEFAULT_SMALL 256 // Default small size
#define MAX_SMALL 512 // Max. small size

/* For is_valid in pictdb_metadata */
#define EMPTY 0
#define NON_EMPTY 1

// pictDB library internal codes for different picture resolutions.
#define RES_THUMB 0
#define RES_SMALL 1
#define RES_ORIG  2
#define NB_RES    3

// accessor for x and y dimensions for different picture dimensions.
#define NB_DIM	    2 // number of dimension for the images (x and y)
#define DIM_X_ORIG  0
#define DIM_Y_ORIG  1
#define DIM_X_THUMB 0
#define DIM_Y_THUMB 1
#define DIM_X_SMALL 2
#define DIM_Y_SMALL 3

// Test if filename is valid
#define TEST_FILENAME(filename) if(strlen(filename) > FILENAME_MAX && strlen(filename) != 0) return ERR_INVALID_FILENAME

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure representing the database's header
 *
 * db_name Database's name
 * db_version Database's version. Increments after each modification
 * num_files Number of picture in database
 * max_files Max number of picture the database can contain
 * res_resized Pictures dimension for thumbnail and small
 * unused_32 Unused yet
 * unused_64 Unused yet
*/
struct pictdb_header {
    char db_name[MAX_DB_NAME + 1];
    uint32_t db_version;
    uint32_t num_files;
    uint32_t max_files;
    uint16_t res_resized[NB_DIM * (NB_RES - 1)];
    uint32_t unused_32;
    uint64_t unused_64;
};

/**
 * @brief Structure representing a picture's metadata
 *
 * pict_id Unique picture's ID
 * SHA Picture's hashcode
 * res_orig Original picture's dimension
 * size Memory size (in bytes) of the picture with different dimension
 * offset Positions of the picture with different dimension in the database file
 * is_valid Indicates if the picture is still used (value NON_EMPTY) or has been deleted (value EMPTY)
 * unused_16 Unused yet
*/
struct pict_metadata {
    char pict_id[MAX_PIC_ID + 1];
    unsigned char SHA[SHA256_DIGEST_LENGTH];
    uint32_t res_orig[NB_DIM];
    uint32_t size[NB_RES];
    uint64_t offset[NB_RES];
    uint16_t is_valid;
    uint16_t unused_16;
};

/**
 * @brief Structure representing a PictDB
 *
 * fpdb Indicates the file containing the data (on disk)
 * header Database's header
 * metadata Metadata of the picture in the database
 */
struct pictdb_file {
    FILE* fpdb;
    struct pictdb_header header;
    struct pict_metadata* metadata;
};

/**
 * @brief Enum representing the output mode for do_list
 *
 * STDOUT for standar output
 * JSON for the the Json-c format
 */
enum do_list_mode {
    STDOUT,
    JSON
};

/**
 * @brief Prints database header informations.
 *
 * @param header The header to be displayed.
 */
void print_header(const struct pictdb_header* header);

/**
 * @brief Prints picture metadata informations.
 *
 * @param metadata The metadata of one picture.
 */
void print_metadata(const struct pict_metadata* metadata);

/**
 * @brief Displays (on stdout) pictDB metadata.
 *
 * @param db_file In memory structure with header and metadata.
 */
const char* do_list(const struct pictdb_file* myfile, enum do_list_mode mode);

/**
 * @brief Creates the database called db_filename. Writes the header and the
 *        preallocated empty metadata array to database file.
 *
 * @param db_filename The name of the file to create.
 * @param db_file In memory structure with header and metadata
 *
 * @return Returns 0 if it succeded or a corresponding error code
 */
int do_create(const char* db_filename, struct pictdb_file* db_file);

/**
 * @brief Opens a file, reads the header & the metadatas
 *				 and checks that there were no problem.
 *
 * @param db_filename The name of the file to read.
 * @param open_mode The type of opening on the file.
 * @param db_file In memory structure with header and metadata
 *
 * @return Returns 0 if it succeded or a corresponding error code
 */
int do_open(const char* db_filename, const char* open_mode, struct pictdb_file* db_file);

/**
* @brief Closes a file and free the metadatas
*
* @param db_file In memory structure with header and metadata
*/
void do_close(struct pictdb_file* db_file);

/**
 * @brief Deletes a picture from the database
 * Does not erase the picture but change the metadata is_valid to EMPTY
 * and adjut the header (db_version +1, num_files -1)
 *
 * @param pict_id The picture we want to delete
 * @param db_file In memory structure with header and metadata
 *
 * @return Returns 0 if it succeded or a corresponding error code
 */
int do_delete(const char* pictID, struct pictdb_file* db_file);

/**
 * @brief Transform a string in one of the resolution constants
 *
 * @param resolution The string specifying the wanted resolution
 *
 * @return Returns the resolution constants corresponding to the string
 */
int resolution_atoi(const char* resolution);

/**
 * @brief Read an image from the pictDB and save it to the buffer if it
 * exists. If the image does not exist in the given dimension, call
 * lazily_resize to create the image.
 *
 * @param pictID picture ID
 * @param dim Internal code corresponding to the dimension we want
 * @param image_buffer address of a byte array (that will contain the picture)
 * @param image_size address of a unsigned 32bit int that will containe the picture's size
 * @param db_file In memory structure with header and metadata
 *
 * @return Returns 0 in case of succes
 */
int do_read(const char* pictID, size_t dim, char** image_buffer, uint32_t* image_size, struct pictdb_file* db_file);

/**
 * @brief Add a new image to a given database
 *
 * @param image The image to add
 * @param size The size of the image
 * @param id Image's identifier
 * @param db_file The database
 *
 * @return Returns 0 if addition went well or error code otherwise
 */
int do_insert(const char* image, size_t size, const char* id, struct pictdb_file* db_file);

/**
 * @brief Copy the database contained in db_file to a new one with the name tmpdb_filename,
 * remove all "hole" in the metadatas and remove all deleted image from the db_file
 *
 * @param db_file The database
 * @param db_filename The name of the database
 * @param tmpdb_filename The name of the temporary database to create
 *
 * @return Returns 0 in case of success
 */
int do_gbcollect(struct pictdb_file* db_file, const char* db_filename, const char* tmpdb_filename);

/**
 * @brief Reads an image from the disk.
 *
 * @param image_buffer Byte array that will contain the picture
 * @param image_size Picture's size
 * @param f The file to read the picture, already opened and with the file
 * position indicator to the correct place
 *
 * @return Returns 0 in case of success
 */
int read_disk_image(char** image_buffer, const uint32_t image_size, FILE* f);

/**
 * @brief Reads an image from a db_file
 *
 * @param image_buffer Byte array that will contain the picture
 * @param index Index of the picture in the db_file
 * @param dim Internal code corresponding to the dimension we want
 * @param db_file The database
 *
 * @return Returns 0 in case of success
 */
int read_db_file_image(char** image_buffer, const size_t index, const size_t dim, const struct pictdb_file* db_file);

/**
 * @brief Gets the size of an image (in bytes).
 *
 * @param image The image file (already open in read mode).
 * @param size A pointer to store the size of the image.
 *
 * @return Returns 0 if the read went well, error code otherwise.
 */
int get_image_size(FILE* image, long* size);

/**
 * @brief Write an image at the end of a db_file and store the offset at which the image is
 *
 * @param image_buffer The image to add
 * @param image_size The image size in bytes
 * @param offset The offset at which the image is written
 * @param db_file The database
 *
 * @return Returns 0 in case of success
 */
int write_db_file_image(const char* image_buffer, const uint32_t image_size, long* offset, struct pictdb_file* db_file);

/**
 * @brief Write an image on the file
 *
 * @param image_buffer The image to add
 * @param image_size The image size in bytes
 * @param f The file in which we write the picture
 *
 * @return Returns 0 in case of success
 */
int write_disk_image(const char* image_buffer, const uint32_t image_size, FILE* f);

/**
 * @brief Get the picture's index corresponding to pictID
 *
 * @param pictID the picture's name
 * @param index the picture's index
 * @param db_file the database
 *
 * @return Returns 0 in case of success
 */
int get_image_index(const char* pictID, size_t* index, const struct pictdb_file* db_file);

/**
 * @brief Write the header on the database file
 *
 * @param db_file The database
 *
 * @return Returns 0 in case of success
 */
int write_db_file_header(const struct pictdb_file* db_file);

/**
 * @brief Write one metadata on the database file
 *
 * @param db_file The database
 * @param index The index of the metadata to write
 *
 * @return Returns 0 in case of success
 */
int write_db_file_one_metadata(const struct pictdb_file* db_file, size_t index);

/**
 * @brief Compares two SHA-hash
 *
 * @param SHA_1 The first SHA-hash.
 * @param SHA_2 The second SHA-hash.
 *
 * @return Returns 0 if both SHAs are the same.
 */
int cmp_SHA(const unsigned char* SHA_1, const unsigned char* SHA_2);

#ifdef __cplusplus
}
#endif
#endif
