/**
 * @file db_list.c
 * @brief Print header of db and then the metadata of all valid images
 * 	or say that the database is empty
 *
 * @author Alexis Montavon and Dorian Laforest
 */

#include "pictDB.h"
#include <json-c/json.h>

#define EMPTY_DATABASE_MSG "<< empty database >>"
#define UNIMP_LIST_MODE_MSG "Unimplemented do_list mode"

/********************************************************************//**
 * Displays (on stdout) pictDB metadata.
 ********************************************************************** */
const char* do_list(const struct pictdb_file* myfile, enum do_list_mode mode)
{
    if(mode == STDOUT) {
        print_header(&myfile->header);
        if(myfile->header.num_files == 0) {
            puts(EMPTY_DATABASE_MSG);
        } else {
            for(size_t i = 0; i < myfile->header.max_files; ++i) {
                if(myfile->metadata[i].is_valid == NON_EMPTY) {
                    print_metadata(&myfile->metadata[i]);
                }
            }
        }
        return NULL;
    } else if(mode == JSON) {
        if(myfile->header.num_files == 0) {
            char* res = calloc(strlen(EMPTY_DATABASE_MSG) + 1, sizeof(char));
            if(NULL != res) {
                strcpy(res, EMPTY_DATABASE_MSG);
            }
            return res;
        } else {
            json_object* jobj = json_object_new_object();
            json_object* pic_array = json_object_new_array();
            for(size_t i = 0; i < myfile->header.max_files; ++i) {
                if(myfile->metadata[i].is_valid == NON_EMPTY) {
                    json_object_array_add(pic_array, json_object_new_string(myfile->metadata[i].pict_id));
                }
            }
            json_object_object_add(jobj, "Pictures", pic_array);
            const char* s = json_object_to_json_string(jobj);
            //copy the string bound to the json object in an other string
            char* res = calloc(strlen(s)+1, sizeof(char));
            if(NULL != res) {
                strcpy(res, s);
            }
            //free the json object
            json_object_put(jobj);
            return res;
        }
    } else {
        char* res = calloc(strlen(UNIMP_LIST_MODE_MSG) + 1, sizeof(char));
        if(NULL != res) {
            strcpy(res, UNIMP_LIST_MODE_MSG);
        }
        return res;
    }
}
