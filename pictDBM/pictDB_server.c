/**
 * @file pictDB_server.c
 * @brief pictDB Server: Web server for pictDB project
 *
 * @author Alexis Montavon and Dorian Laforest
 */

#include "../libmongoose/mongoose.h"
#include "error.h"
#include "pictDB.h"
#include "image_content.h"

#define MAX_QUERY_PARAM 5

static const char* http_port = "8000";
static struct mg_serve_http_opts server_opts;
static int sig_received = 0;

static void signal_handler(int sig_num)
{
    signal(sig_num, signal_handler);
    sig_received = sig_num;
}

/**
 * @brief Query_string parser
 *
 * @param result An array of char pointer of size MAX_QUERY_PARAM.
 * @param tmp String that contains every part of the Query_string.
 * @param src The Query_string to split.
 * @param delim String containing all char splitting the string.
 * @param len Size of the Query_string.
 */
void split(char* result[], char* tmp, const char* src, const char* delim, size_t len)
{
    char str[len + 1];
    strncpy(str, src, len);
    str[len] = '\0';
    tmp = strtok(str, delim);
    for(size_t i = 0; i < MAX_QUERY_PARAM; ++i) {
        if(tmp != NULL) {
            result[i] = calloc(strlen(tmp) + 1, sizeof(char));
            if(NULL != result[i]) {
                strncpy(result[i], tmp, strlen(tmp) + 1);
            }
            tmp = strtok(NULL, delim);
        } else {
            result[i] = NULL;
        }
    }
}

/**
 * @brief Send an HTTP error message.
 *
 * @param nc The mongoose connection.
 * @param error The error code.
 */
void mg_error(struct mg_connection* nc, int error)
{
    mg_printf(nc, "HTTP/1.1 500 %s\r\n", ERROR_MESSAGES[error]);
    mg_printf(nc, "Content-Length: %d\r\n\r\n", 0);
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

/**
 * @brief Handles list call on server, send http message
 *        with the result of the JSON do_list.
 *
 * @param nc The mongoose connection.
 */
static void handle_list_call(struct mg_connection* nc)
{
    const char* pict_list = do_list(nc->mgr->user_data, JSON);
    if(pict_list == NULL) {
        mg_error(nc, ERR_IO);
    } else {
        size_t size = strlen(pict_list);
        mg_printf(nc, "HTTP/1.1 200 OK\r\n");
        mg_printf(nc, "Content-Type: application/json\r\n");
        mg_printf(nc, "Content-Length: %zu\r\n\r\n", size);
        mg_printf(nc, "%s", pict_list);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        free((void*)pict_list);
    }
}

/**
 * @brief get the ID and res (if it exists) in the query_string
 *
 * @param query_string The mg_string containing the query to parse
 * @param res Pointer to the resolution that will be set if res is found
 * @param id Pointer to the id that will be set if id is found
 */
static void get_ID_and_RES(struct mg_str query_string, int* res, char** id)
{
    // Get image ID and wanted resolution using split function.
    char* result[MAX_QUERY_PARAM] = {NULL, NULL, NULL, NULL, NULL};
    char tmp[(MAX_PIC_ID + 1) * MAX_QUERY_PARAM] = "";

    split(result, tmp, query_string.p, "&=", query_string.len);

    for(size_t i = 0; i < MAX_QUERY_PARAM; ++i) {
        if(result[i] != NULL) {
            if(!strcmp(result[i], "res") && (i < MAX_QUERY_PARAM-1) && (NULL != result[i+1])) {
                *res = resolution_atoi(result[i+1]);
            } else if(!strcmp(result[i], "pict_id") && (i < MAX_QUERY_PARAM-1) && (NULL != result[i+1])) {
                size_t len = strlen(result[i+1]);
                *id = calloc(len + 1, sizeof(char));
                if(NULL != *id) {
                    strncpy(*id, result[i+1], len + 1);
                }
            }
            free(result[i]);
            result[i] = NULL;
        }
    }
}


/**
 * @brief Handles read call on server, splits http query using split function,
 *        reads and sends the image using do_read.
 *
 * @param nc The mongoose connection.
 * @param hm The http message relative to the event.
 */
static void handle_read_call(struct mg_connection* nc, struct http_message* hm)
{
    // Get image ID and wanted resolution using split function.
    int res = -1;
    char* id = NULL;
    get_ID_and_RES(hm->query_string, &res, &id);

    // Check pict ID and res, then reads the image using do_read.
    // If all went well send response with image to browser.
    if(NULL == id) {
        mg_error(nc, ERR_INVALID_ARGUMENT);
    } else {
        size_t pictIDSize = strlen(id);
        if(pictIDSize > MAX_PIC_ID || pictIDSize == 0) {
            free(id);
            mg_error(nc, ERR_INVALID_PICID);
        } else if(res == -1) {
            free(id);
            mg_error(nc, ERR_INVALID_ARGUMENT);
        } else {
            char* image_buffer = NULL;
            uint32_t image_size = 0;
            int errCode = do_read(id, res, &image_buffer, &image_size, nc->mgr->user_data);
            free(id);
            id = NULL;
            if(0 != errCode) {
                mg_error(nc, errCode);
            } else {
                mg_printf(nc, "HTTP/1.1 200 OK\r\n");
                mg_printf(nc, "Content-Type: image/jpeg\r\n");
                mg_printf(nc, "Content-Length: %u\r\n\r\n", image_size);
                mg_send(nc, image_buffer, image_size);
                nc->flags |= MG_F_SEND_AND_CLOSE;
                free(image_buffer);
            }
        }
    }
}

/**
 * @brief Handles insert call on server, retrieve image name and content within the POST
 * information using mg_parse_multipart and then call do_insert to add the image into the
 * database.
 *
 * @param nc The mongoose connection.
 * @param hm The http message relative to the event.
 */
static void handle_insert_call(struct mg_connection* nc, struct http_message* hm)
{
    char var_name[100], file_name[100];
    const char *chunk;
    size_t chunk_len, n1;

    n1 = 0;
    if(0 == hm->body.len) {
        mg_error(nc, ERR_INVALID_ARGUMENT);
    } else {
        mg_parse_multipart(hm->body.p + n1, hm->body.len - n1, var_name, sizeof(var_name), file_name, sizeof(file_name), &chunk, &chunk_len);

        int errCode = do_insert(chunk, chunk_len, file_name, nc->mgr->user_data);

        if(0 != errCode) {
            mg_error(nc, errCode);
        } else {
            mg_printf(nc, "HTTP/1.1 302 Found\r\n");
            mg_printf(nc, "Location: http://localhost:%s/index.html\r\n\r\n", http_port);
            nc->flags |= MG_F_SEND_AND_CLOSE;
        }
    }
}

/**
 * @brief Handles delete call on server, splits http query using split function,
 *        delete the image using do_delete.
 * @param nc The mongoose connection.
 * @param hm The http message relative to the event.
 */
static void handle_delete_call(struct mg_connection* nc, struct http_message* hm)
{
    int res = -1;
    char* id = NULL;
    get_ID_and_RES(hm->query_string, &res, &id);

    // Check pict ID, then reads the image using do_read.
    // If all went well send response with image to browser.
    if(NULL == id) {
        mg_error(nc, ERR_INVALID_ARGUMENT);
    } else {
        size_t pictIDSize = strlen(id);
        if(pictIDSize > MAX_PIC_ID || pictIDSize == 0) {
            free(id);
            id = NULL;
            mg_error(nc, ERR_INVALID_PICID);
        } else {
            int errCode = do_delete(id, nc->mgr->user_data);
            free(id);
            id = NULL;
            if(0 != errCode) {
                mg_error(nc, errCode);
            } else {
                mg_printf(nc, "HTTP/1.1 302 Found\r\n");
                mg_printf(nc, "Location: http://localhost:%s/index.html\r\n\r\n", http_port);
                nc->flags |= MG_F_SEND_AND_CLOSE;
            }
        }
    }
}

static void ev_handler(struct mg_connection* nc, int ev, void* event_data)
{
    struct http_message* hm = (struct http_message*) event_data;
    switch(ev) {
    case MG_EV_HTTP_REQUEST:
        if (mg_vcmp(&hm->uri, "/pictDB/list") == 0) {
            handle_list_call(nc);
        } else if(mg_vcmp(&hm->uri, "/pictDB/read") == 0) {
            handle_read_call(nc, hm);
        } else if(mg_vcmp(&hm->uri, "/pictDB/insert") == 0) {
            handle_insert_call(nc, hm);
        } else if(mg_vcmp(&hm->uri, "/pictDB/delete") == 0) {
            handle_delete_call(nc, hm);
        } else {
            mg_serve_http(nc, hm, server_opts); /* Serve static content */
        }
        break;
    default:
        break;
    }
}

int main(int args, char* argv[])
{
    int ret = 0;
    if(args != 2) {
        ret = ERR_INVALID_ARGUMENT;
    } else if(VIPS_INIT(argv[0])) {
        ret = ERR_VIPS;
    } else {
        // Skip program name
        argv++;
        args--;

        // Test db_name
        char* db_filename = argv[0];
        TEST_FILENAME(db_filename);
        struct pictdb_file db_file;
        ret = do_open(db_filename, "rb+", &db_file);
        if(0 != ret) {
            vips_shutdown();
            fprintf(stderr, "ERROR: %s\n", ERROR_MESSAGES[ret]);
        } else {
            print_header(&db_file.header);

            // Server part:
            struct mg_mgr mgr;
            struct mg_connection* nc;
            signal(SIGTERM, signal_handler);
            signal(SIGINT, signal_handler);
            mg_mgr_init(&mgr, &db_file);

            nc = mg_bind(&mgr, http_port, ev_handler);
            if (NULL == nc) {
                do_close(&db_file);
                vips_shutdown();
                fprintf(stderr, "Error starting server on port %s\n", http_port);
                exit(EXIT_FAILURE);
            }

            // Set up HTTP server parameters
            mg_set_protocol_http_websocket(nc);
            server_opts.document_root = ".";  // Serve current directory
            printf("Starting PictDB_server on port %s\n", http_port);

            while (!sig_received) {
                mg_mgr_poll(&mgr, 1000);
            }
            do_close(&db_file);
            mg_mgr_free(&mgr);
            vips_shutdown();
            return 0;
        }
    }
}
