/**
 * @file db_delete.c
 * @brief pictDB library: do_delete implementation.
 *
 * @author Alexis Montavon and Dorian Laforest
 */

#include "pictDB.h"

/********************************************************************//**
 * Deletes the picture referenced by pict_id in the database db_file.
 *
 */
int do_delete(const char* pictID, struct pictdb_file* db_file)
{
    //In this project, pictDBM, we made the decision to return the error
    //ERR_INVALID_ARGUMENT because we did not found a better one and
    //because this error should not occur since the argument are already
    //tested before the call of this function. But if this function is used
    //in an other library, the argument should be tested
    if((NULL == pictID) || (NULL == db_file) || (NULL == db_file->metadata)) {
        return ERR_INVALID_ARGUMENT;
    }

    if(0 == db_file->header.num_files) {
        return ERR_FILE_NOT_FOUND;
    }

    //Pointer to the metadata referenced by pict_id
    struct pict_metadata* pict_to_delete = NULL;

    //Search for pict_id
    //i is the index of the picture to delete
    size_t i = 0;
    if(0 != get_image_index(pictID, &i, db_file)) {
        return ERR_FILE_NOT_FOUND;
    }
    pict_to_delete = &db_file->metadata[i];

    //Modify is_valid
    pict_to_delete->is_valid = EMPTY;

    //Write the modified metadata on disk
    if(0 != write_db_file_one_metadata(db_file, i)) {
        return ERR_IO;
    }

    //Modifiy header
    db_file->header.db_version += 1;
    db_file->header.num_files -= 1;

    //Write header on disk
    if(0 != write_db_file_header(db_file)) {
        return ERR_IO;
    }
    return 0;
}
