/**
 * @file db_create.c
 * @brief pictDB library: do_create implementation.
 *
 * @author Mia Primorac
 * @date 2 Nov 2015
 *
 * Last modification :
 * @author Alexis Montavon and Dorian Laforest
 */

#include "pictDB.h"

/********************************************************************//**
 * Creates the database called db_filename. Writes the header and the
 * preallocated empty metadata array to database file.
 */
int do_create(const char* db_filename, struct pictdb_file* db_file)
{
    //Test the pointer
    //In this project, pictDBM, we made the decision to return the error
    //ERR_INVALID_ARGUMENT because we did not found a better one and
    //because this error should not occur since the argument are already
    //tested before the call of this function. But if this function is used
    //in an other library, the argument should be tested
    if((NULL == db_filename) || (NULL == db_file)) {
        return ERR_INVALID_ARGUMENT;
    }
    //Initialise the DB header
    strncpy(db_file->header.db_name, CAT_TXT,  MAX_DB_NAME);
    db_file->header.db_name[MAX_DB_NAME] = '\0';
    db_file->header.db_version = 0;
    db_file->header.num_files = 0;
    db_file->header.unused_32 = 0;
    db_file->header.unused_64 = 0;

    //Allocating dynamiclly the DB metadata
    db_file->metadata = calloc(db_file->header.max_files, sizeof(struct pict_metadata));
    if(db_file->metadata == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    //Initialise DB metadata
    for(size_t i = 0; i < db_file->header.max_files; ++i) {
        db_file->metadata[i].is_valid = EMPTY;
    }

    //Initialise the DB fpdb
    db_file->fpdb = NULL;

    // Creates database called db_filename
    db_file->fpdb = fopen(db_filename, "wb+");
    if(db_file->fpdb == NULL) {
        //Free the metadatas
        do_close(db_file);
        return ERR_IO;
    } else {
        if(0 != write_db_file_header(db_file)) {
            do_close(db_file);
            return ERR_IO;
        }
        if(fwrite(db_file->metadata, sizeof(struct pict_metadata), db_file->header.max_files, db_file->fpdb) != db_file->header.max_files) {
            //error with fwrite
            do_close(db_file);
            return ERR_IO;
        }
        return 0;
    }
}
