/**
 * @file dedup.c
 * @brief Implementation of do_name_and_content_dedup.
 *
 * @author Alexis Montavon and Dorian Laforest
 */

#include "dedup.h"

/********************************************************************//**
* Function to prevent duplication of images in database.
************************************************************************/
int do_name_and_content_dedup(struct pictdb_file* db_file, uint32_t index)
{
    //In this project, pictDBM, we made the decision to return the error
    //ERR_INVALID_ARGUMENT because we did not found a better one and
    //because this error should not occur since the argument are already
    //tested before the call of this function. But if this function is used
    //in an other library, the argument should be tested
    if((NULL == db_file) || (index > db_file->header.max_files)) {
        return ERR_INVALID_ARGUMENT;
    } else {
        const char* id = db_file->metadata[index].pict_id;
        unsigned char* SHA = db_file->metadata[index].SHA;
        db_file->metadata[index].offset[RES_ORIG] = 0;
        for(size_t i = 0; i < db_file->header.max_files; ++i) {
            if(i != index && db_file->metadata[i].is_valid == NON_EMPTY) {
                if(strcmp(db_file->metadata[i].pict_id, id) == 0) {
                    return ERR_DUPLICATE_ID;
                } else if(cmp_SHA(db_file->metadata[i].SHA, SHA) == 0) {
                    db_file->metadata[index].size[RES_THUMB] = db_file->metadata[i].size[RES_THUMB];
                    db_file->metadata[index].size[RES_SMALL] = db_file->metadata[i].size[RES_SMALL];
                    db_file->metadata[index].offset[RES_ORIG] = db_file->metadata[i].offset[RES_ORIG];
                    db_file->metadata[index].offset[RES_THUMB] = db_file->metadata[i].offset[RES_THUMB];
                    db_file->metadata[index].offset[RES_SMALL] = db_file->metadata[i].offset[RES_SMALL];
                    db_file->metadata[index].res_orig[DIM_X_ORIG] = db_file->metadata[i].res_orig[DIM_X_ORIG];
                    db_file->metadata[index].res_orig[DIM_Y_ORIG] = db_file->metadata[i].res_orig[DIM_Y_ORIG];
                }
            }
        }
        return 0;
    }
}
