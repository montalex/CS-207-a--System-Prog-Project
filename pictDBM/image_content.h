/**
 * @file error.h
 * @brief pictDB library image content.
 *
 * @author Alexis Montavon and Dorian Laforest
 */
#ifndef PICTDBPRJ_IMAGE_CONTENT_H
#define PICTDBPRJ_IMAGE_CONTENT_H

#include <vips/vips.h>
#include "pictDB.h"
#include "dedup.h"
/**
 * @brief Resizes the image specified by index into the specified dimension
 *
 * @param dim Internal code corresponding to the dimension we want
 * @param db_file In memory structure with header and metadata
 * @param index Position of the image to process
 *
 * @return Returns 0 in case of succes (or if res == RES_ORIG and nothing is done)
 */
size_t lazily_resize(size_t dim, struct pictdb_file* db_file, size_t index);

/**
 * @brief Gets the picture's width and height using the VIPS library and
 * stocks it in the height and width parameters
 *
 * @param height Picture's height we want to get
 * @param width Pictures's width we want to get
 * @param image_buffer Pointer to the memory region containing a JPEG picture
 * @param image_size Size of the region pointed by image_buffer
 *
 * @return Returns 0 in case of succes, ERR_VIPS otherwise
 */
int get_resolution(uint32_t* height         ,
                   uint32_t* width          ,
                   const char* image_buffer ,
                   size_t image_size        );

#endif //PICTDBPRJ_IMAGE_CONTENT_H
