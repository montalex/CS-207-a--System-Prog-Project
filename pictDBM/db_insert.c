/**
 * @file db_insert.c
 * @brief Implementation of do_insert.
 *
 * @author Alexis Montavon and Dorian Laforest
 */

#include "pictDB.h"
#include "dedup.h"
#include "image_content.h"

/********************************************************************//**
 * Adds the image to the database db_file.
 */
int do_insert(const char* image, size_t im_size, const char* id, struct pictdb_file* db_file)
{
    //In this project, pictDBM, we made the decision to return the error
    //ERR_INVALID_ARGUMENT because we did not found a better one and
    //because this error should not occur since the argument are already
    //tested before the call of this function. But if this function is used
    //in an other library, the argument should be tested
    if((NULL == image) || (NULL == id) || (NULL == db_file)) {
        return ERR_INVALID_ARGUMENT;
    } else if(db_file->header.num_files < db_file->header.max_files) {
        uint32_t index = 0;
        while(db_file->metadata[index].is_valid != EMPTY) {
            index += 1;
        }
        // Add image's SHA value
        (void) SHA256((unsigned char *)image, im_size, db_file->metadata[index].SHA);
        // Add image's id
        strncpy(db_file->metadata[index].pict_id, id, MAX_PIC_ID);
        db_file->metadata[index].pict_id[MAX_PIC_ID] = '\0';
        // Add image's original size and initialise others to 0.
        db_file->metadata[index].size[RES_ORIG] = im_size;
        // Test deduplication and write if no duplication
        int error_code = do_name_and_content_dedup(db_file, index);
        if(error_code != 0) {
            return error_code;
        } else {
            // Test if image isn't there yet.
            if(db_file->metadata[index].offset[RES_ORIG] == 0) {
                uint32_t height = 0;
                uint32_t width = 0;
                error_code = get_resolution(&height, &width, image, im_size);
                if(error_code != 0) {
                    return error_code;
                }
                long offset = 0;
                if(0 != write_db_file_image(image, im_size, &offset, db_file)) {
                    return ERR_IO;
                }
                db_file->metadata[index].size[RES_THUMB] = 0;
                db_file->metadata[index].size[RES_SMALL] = 0;
                db_file->metadata[index].offset[RES_ORIG] = offset;
                db_file->metadata[index].offset[RES_THUMB] = 0;
                db_file->metadata[index].offset[RES_SMALL] = 0;
                db_file->metadata[index].res_orig[DIM_X_ORIG] = width;
                db_file->metadata[index].res_orig[DIM_Y_ORIG] = height;
            }
            db_file->metadata[index].is_valid = NON_EMPTY;
            db_file->header.db_version += 1;
            db_file->header.num_files += 1;
            //Updates header on disk
            if(0 != write_db_file_header(db_file)) {
                return ERR_IO;
            }
            if(0 != write_db_file_one_metadata(db_file, index)) {
                return ERR_IO;
            }
            return 0;
        }
    } else {
        return ERR_FULL_DATABASE;
    }
}
