/**
 * @file db_gbcollect.c
 * @brief pictDB library: do_gbcollect implementation
 *
 * @author Alexis Montavon and Dorian Laforest
 */

#include "image_content.h"

/********************************************************************//**
 * Create a new database file without the deleted image
 */
int do_gbcollect(struct pictdb_file* db_file, const char* db_filename, const char* tmpdb_filename)
{
    //Testing arguments
    if((NULL == db_filename) || (NULL == tmpdb_filename) || (NULL == db_file)) {
        return ERR_INVALID_ARGUMENT;
    }

    //Opening and creating a temporary db_file
    struct pictdb_file tmpdb_file;
    tmpdb_file.header.max_files = db_file->header.max_files;
    tmpdb_file.header.res_resized[DIM_X_THUMB] = db_file->header.res_resized[DIM_X_THUMB];
    tmpdb_file.header.res_resized[DIM_Y_THUMB] = db_file->header.res_resized[DIM_Y_THUMB];
    tmpdb_file.header.res_resized[DIM_X_SMALL] = db_file->header.res_resized[DIM_X_SMALL];
    tmpdb_file.header.res_resized[DIM_Y_SMALL] = db_file->header.res_resized[DIM_Y_SMALL];

    int errorCode = 0; //0 means no error
    errorCode = do_create(tmpdb_filename, &tmpdb_file);
    if(errorCode != 0) {
        return errorCode;
    }

    //Transfering image to the temporary database
    size_t newIndex = 0;
    for(size_t index = 0; index < db_file->header.max_files; ++index) {
        if(NON_EMPTY == db_file->metadata[index].is_valid) {
            const char* pictID = db_file->metadata[index].pict_id;

            //Adds image in original format
            char* image_buffer = NULL;
            uint32_t image_size = 0;

            //read the image in original format
            errorCode = do_read(pictID, RES_ORIG, &image_buffer, &image_size, db_file);
            if(0 != errorCode) {
                do_close(&tmpdb_file);
                return errorCode;
            }

            //insert the image
            errorCode = do_insert(image_buffer, image_size, pictID, &tmpdb_file);
            free(image_buffer);
            image_buffer = NULL;
            if(0 != errorCode) {
                do_close(&tmpdb_file);
                return errorCode;
            }

            //Adds image in small format if it exists in original db_file
            if(0 != db_file->metadata[index].size[RES_SMALL]) {
                errorCode = lazily_resize(RES_SMALL, &tmpdb_file, newIndex);
                if(0 != errorCode) {
                    do_close(&tmpdb_file);
                    return errorCode;
                }
            }

            //Adds image in thumb format if it exists in original db_file
            if(0 != db_file->metadata[index].size[RES_THUMB]) {
                errorCode = lazily_resize(RES_THUMB, &tmpdb_file, newIndex);
                if(0 != errorCode) {
                    do_close(&tmpdb_file);
                    return errorCode;
                }
            }
            newIndex++;
        }
    }

    do_close(&tmpdb_file);

    if(remove(db_filename) != 0) {
        return ERR_IO;
    } else if(rename(tmpdb_filename, db_filename) != 0) {
        return ERR_IO;
    }

    return 0;
}
