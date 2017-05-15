/**
 * @file error.h
 * @brief pictDB library error codes and messages.
 *
 * @author Mia Primorac
 * @date 2 Nov 2015
 */

#ifndef PICTDBPRJ_ERROR_H
#define PICTDBPRJ_ERROR_H

/**
 * @brief pictDB library internal error codes.
 *
 */
enum error_codes {
    ERR_IO = 1,
    ERR_OUT_OF_MEMORY,
    ERR_NOT_ENOUGH_ARGUMENTS,
    ERR_INVALID_FILENAME,
    ERR_INVALID_COMMAND,
    ERR_INVALID_ARGUMENT,
    ERR_MAX_FILES,
    ERR_RESOLUTIONS,
    ERR_INVALID_PICID,
    ERR_FULL_DATABASE,
    ERR_FILE_NOT_FOUND,
    NOT_IMPLEMENTED,
    ERR_DUPLICATE_ID,
    ERR_VIPS,
    ERR_DEBUG
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief pictDB library internal error messages.
 *
 */
extern
const char * const ERROR_MESSAGES[];

#ifdef __cplusplus
}
#endif
#endif
