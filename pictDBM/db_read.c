/**
 * @file db_read.c
 * @brief pictDB library: do_read implementation
 *
 * @author Alexis Montavon and Dorian Laforest
 */

#include "image_content.h"

/********************************************************************//**
 * Read an image from the pictDB and save it to the buffer if it exists.
 * If the image does not exist in the given dimension, call lazily_resize
 * to create the image.
 */
int do_read(const char* pictID,
            size_t dim,
            char** image_buffer,
            uint32_t* image_size,
            struct pictdb_file* db_file)
{
    //Test argument
    //In this project, pictDBM, we made the decision to return the error
    //ERR_INVALID_ARGUMENT because we did not found a better one and
    //because this error should not occur since the argument are already
    //tested before the call of this function. But if this function is used
    //in an other library, the argument should be tested
    if((NULL == pictID) || (NULL == db_file)) {
        return ERR_INVALID_ARGUMENT;
    }
    //Test dimension argument
    if((RES_THUMB != dim) && (RES_SMALL != dim) && (RES_ORIG != dim)) {
        return ERR_RESOLUTIONS;
    }
    //Search pictID in metadata
    int errorCode = 0;
    size_t i = 0;
    errorCode = get_image_index(pictID, &i, db_file);
    if(errorCode != 0) {
        return errorCode;
    }

    //Test if the image exists in given dimension -> lazily_resize
    errorCode = lazily_resize(dim, db_file, i);
    if(0 != errorCode) {
        return errorCode;
    }

    //We know the size and the position in file -> dynamic allocation of buffer and load the image in it
    //Load the size in the image_size
    *image_size = db_file->metadata[i].size[dim];
    return read_db_file_image(image_buffer, i, dim, db_file);
}
