/**
 * @file pictDBM_tools.c
 * @brief pictDB Manager : useful tool functions
 *
 * @author Jean-Cédric Chappelier
 * @date 25 Nov 2015
 */

#include <stdint.h> // for uint16_t, uint32_t
#include <errno.h>
#include <inttypes.h> // strtoumax()

/********************************************************************//**
 * Tool functions for string to uint<N>_t conversion.
 ********************************************************************** */
#define define_atouintN(N) \
uint ## N ## _t \
atouint ## N(const char* str) \
{ \
    char* endptr; \
    errno = 0; \
    uintmax_t val = strtoumax(str, &endptr, 10); \
    if (errno == ERANGE || val > UINT ## N ## _MAX \
        || endptr == str || *endptr != '\0') { \
        errno = ERANGE; \
        return 0; \
    } \
    return (uint ## N ## _t) val; \
}

define_atouintN(16)
define_atouintN(32)
