/**
 * @file image_content.c
 * @brief pictDB library: lazily resize implementation
 *
 * @author Alexis Montavon and Dorian Laforest
 */

#include "image_content.h"

/**
 * @brief Get the index of all image that have the same content of the image at metadata[index] and updates it if needed
 *
 * @param index_tab Pointer to an array that will contain all the index's of duplicate image
 * @param db_file The database
 * @param original The original image in VIPS format
 *
 * @return Returns the scaling ratio
 */
static int get_dup_index_and_update(size_t** index_tab, size_t* size_tab, const struct pictdb_file* db_file, const size_t index)
{
    size_t size_tab_max = 10;
    *index_tab = calloc(size_tab_max, sizeof(size_t));
    if(NULL == *index_tab) {
        return ERR_OUT_OF_MEMORY;
    }
    (*index_tab)[0] = index;
    *size_tab = 1;
    unsigned char* SHA = db_file->metadata[index].SHA;
    for(size_t i = 0; i < db_file->header.max_files; ++i) {
        if(i != index && NON_EMPTY == db_file->metadata[i].is_valid) {
            if(0 == cmp_SHA(db_file->metadata[i].SHA, SHA)) {
                if(*size_tab >= size_tab_max) {
                    *index_tab = realloc(*index_tab, 2*size_tab_max*sizeof(size_t));
                    size_tab_max*= 2;
                    if(NULL == *index_tab) {
                        return ERR_OUT_OF_MEMORY;
                    }
                }
                (*index_tab)[*size_tab] = i;
                ++(*size_tab);
            }
        }
    }
    //Updates the metadata at index if needed
    if(*size_tab > 1) {
        if(db_file->metadata[index].size[RES_SMALL] != db_file->metadata[(*index_tab)[1]].size[RES_SMALL]) {
            db_file->metadata[index].size[RES_SMALL] = db_file->metadata[(*index_tab)[1]].size[RES_SMALL];
            db_file->metadata[index].offset[RES_SMALL] = db_file->metadata[(*index_tab)[1]].offset[RES_SMALL];
        }
        if(db_file->metadata[index].size[RES_THUMB] != db_file->metadata[(*index_tab)[1]].size[RES_THUMB]) {
            db_file->metadata[index].size[RES_THUMB] = db_file->metadata[(*index_tab)[1]].size[RES_THUMB];
            db_file->metadata[index].offset[RES_THUMB] = db_file->metadata[(*index_tab)[1]].offset[RES_THUMB];
        }
    }
    return 0;
}


/**
 * @brief Computes the scaling ratio from the original image to the image in the desired dimension
 *
 * @param dim Internal code representing the desired dimension
 * @param db_file The database
 * @param original The original image in VIPS format
 *
 * @return Returns the scaling ratio
 */
static const double compute_scaling_ratio(const size_t dim, const struct pictdb_file* db_file, const VipsImage* original)
{
    double h_ratio;
    double v_ratio;
    switch(dim) {
    case RES_THUMB:
        h_ratio = (double) db_file->header.res_resized[DIM_X_THUMB];
        v_ratio = (double) db_file->header.res_resized[DIM_Y_THUMB];
        break;
    case RES_SMALL:
        h_ratio = (double) db_file->header.res_resized[DIM_X_SMALL];
        v_ratio = (double) db_file->header.res_resized[DIM_Y_SMALL];
        break;
    }
    h_ratio /= (double) original->Xsize;
    v_ratio /= (double) original->Ysize;
    const double ratio = h_ratio > v_ratio ? v_ratio : h_ratio;
    return ratio;
}

/**
 * @brief Resize and save into a buffer the image
 *
 * @param buff A buffer containing the original image
 * @param index The image's index in metadatas
 * @param dim Internal code representing the desired dimension
 * @param db_file The database
 * @param outBuffer Pointer to a buffer that will contains the resized image, allocated by VIPS
 * @param newSizeAfterResize Pointer to a size that will contain the size of the resized image
 *
 * @return Returns 0 in case of success
 */
static int resize_and_save_image(void* buff, size_t index, size_t dim, const struct pictdb_file* db_file, void** outBuffer, size_t* newSizeAfterResize)
{
    VipsImage* original;
    // some place to do the job
    VipsObject* process = VIPS_OBJECT(vips_image_new());
    // we want 1 new images
    VipsImage** newImage = (VipsImage**) vips_object_local_array(process, 1);

    if(0 != vips_jpegload_buffer(buff, db_file->metadata[index].size[RES_ORIG], &original, NULL)) {
        g_object_unref(process);
        return ERR_VIPS;
    }

    //Computes the scaling ratio
    const double ratio = compute_scaling_ratio(dim, db_file, original);

    //Resizes the picture
    if(0 != vips_resize(original, &newImage[0], ratio, NULL)) {
        g_object_unref(process);
        return ERR_VIPS;
    }

    //Save the resized picture into a buffer in memory
    *outBuffer = NULL;
    if(0 != vips_jpegsave_buffer(newImage[0], outBuffer, newSizeAfterResize, NULL)) {
        if(NULL != *outBuffer) {
            g_free(*outBuffer);
        }
        g_object_unref(process);
        return ERR_VIPS;
    }
    g_object_unref(process);
    return 0;
}

/********************************************************************//**
 * Resizes the image specified by index into the specified dimension
 */
size_t lazily_resize(size_t dim, struct pictdb_file* db_file, size_t index)
{
    //Test parameters
    if(RES_ORIG == dim) {
        return 0;
    } else if((RES_THUMB != dim) && (RES_SMALL != dim)) {
        return ERR_RESOLUTIONS;
    }

    //In this project, pictDBM, we made the decision to return the error
    //ERR_INVALID_ARGUMENT because we did not found a better one and
    //because this error should not occur since the argument are already
    //tested before the call of this function. But if this function is used
    //in an other library, the argument should be tested
    if(NULL == db_file) {
        return ERR_INVALID_ARGUMENT;
    }

    if((index > db_file->header.num_files) && (EMPTY == db_file->metadata[index].is_valid)) {
        return ERR_INVALID_ARGUMENT;
    }

    int errorCode = 0;
    size_t* index_tab = NULL;
    size_t size_tab = 0;
    errorCode = get_dup_index_and_update(&index_tab, &size_tab, db_file, index);
    if(0 != errorCode) {
        return errorCode;
    }

    //Test if the image exists in the wanted dimension
    if(0 != db_file->metadata[index].size[dim]) {
        if(index_tab != NULL) {
            free(index_tab);
        }
        return 0;
    }

    //Load the original picture in memory
    char* buff = NULL;
    errorCode = read_db_file_image(&buff, index, RES_ORIG, db_file);
    if(0 != errorCode) {
        if(index_tab != NULL) {
            free(index_tab);
        }
        return errorCode;
    }
    //resizing the image
    //we don't need buff anymore
    void* outBuffer = NULL;
    size_t newSizeAfterResize;
    errorCode = resize_and_save_image(buff, index, dim, db_file, &outBuffer, &newSizeAfterResize);
    free(buff);
    if(0 != errorCode) {
        if(index_tab != NULL) {
            free(index_tab);
        }
        if(NULL != outBuffer) {
            g_free(outBuffer);
        }
        return errorCode;
    }
    //write image at the end of the file
    long offset = 0;
    errorCode = write_db_file_image(outBuffer, newSizeAfterResize, &offset, db_file);
    if(0 != errorCode) {
        if(index_tab != NULL) {
            free(index_tab);
        }
        g_free(outBuffer);
        return ERR_IO;
    }

    //Update datas on memory
    for(size_t i = 0; i < size_tab; ++i) {
        db_file->metadata[index_tab[i]].size[dim] = newSizeAfterResize;
        db_file->metadata[index_tab[i]].offset[dim] = offset;
        if(0 != write_db_file_one_metadata(db_file, index_tab[i])) {
            if(index_tab != NULL) {
                free(index_tab);
            }
            g_free(outBuffer);
            return ERR_IO;
        }
    }

    if(index_tab != NULL) {
        free(index_tab);
    }
    g_free(outBuffer);
    return 0;
}

int get_resolution(uint32_t* height         ,
                   uint32_t* width          ,
                   const char* image_buffer ,
                   size_t image_size        )
{
    VipsImage* image;

    if(0 != vips_jpegload_buffer((void*)image_buffer, image_size, &image, NULL)) {
        return ERR_VIPS;
    }
    *height = image->Ysize;
    *width = image->Xsize;
    g_object_unref(image);
    return 0;
}
