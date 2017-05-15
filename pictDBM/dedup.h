/**
 * @file dedup.h
 * @brief useful function to prevent duplication of images.
 *
 * @author Alexis Montavon and Dorian Laforest
 * @date 29 Avr 2016
 */
#ifndef PICTDBPRJ_DEDUP_H
#define PICTDBPRJ_DEDUP_H

#include "pictDB.h"

/**
 * @brief Checks that no two images have the same name (pictID)
 *        or the same SHA.
 *
 * @param file The file in which we want to check (already opened).
 * @param index Specify the position of an image in the metadatas.
 *
 * @return Returns 0 if no duplications or dedup succesful.
 */
int do_name_and_content_dedup(struct pictdb_file* db_file, uint32_t index);

#endif //PICTDBPRJ_DEDUP_H
