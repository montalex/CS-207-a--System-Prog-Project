/**
 * @file pictDBM.c
 * @brief pictDB Manager: command line interpretor for pictDB core commands.
 *
 * Picture Database Management Tool
 *
 * @author Mia Primorac
 * @date 2 Nov 2015
 *
 * Last modification :
 * @author Alexis Montavon and Dorian Laforest
 */

#include "pictDB.h"
#include "image_content.h"
#include "pictDBM_tools.h"

#define N_COMMANDS 7 // Number of commands available, useful for array.

typedef int (*command)(int args, char *argv[]);

typedef struct {
    const char* name;
    command cmd;
} command_mapping;

/**
 * @brief Tool function:
 *        Go to next argument and decrement number of args.
 *
 * @param args Pointer to number of arguments
 * @param argv Pointer on arguments
 */
static void next_arg(int* args, char** argv[])
{
    *args -= 1;
    *argv +=1;
}

/**
 * @brief Utility function to create the picture name :
 * 		original_prefix + resolution_suffix + '.jpg'
 * 	with : - original_prefix = pictID
 * 		   - resolution_suffix = '_orig', '_small' or '_thumb'
 *
 * @param pictID Picture ID
 * @param dim Internal code corresponding to the dimension we want
 *
 * @return The generated picture name
 */
static char* create_name(const char* pictID, size_t dim)
{
    const char* ext = ".jpeg";
    char* resolution_suffix;
    switch(dim) {
    case RES_ORIG:
        resolution_suffix = "_orig";
        break;
    case RES_SMALL:
        resolution_suffix = "_small";
        break;
    case RES_THUMB:
        resolution_suffix = "_thumb";
        break;
    }
    size_t str_size = strlen(pictID) + strlen(ext) + strlen(resolution_suffix);
    char* c = calloc(str_size + 1, sizeof(char));
    strncpy(c, pictID, strlen(pictID)+1);
    strncat(c, resolution_suffix, strlen(resolution_suffix)+1);
    strncat(c, ext, strlen(ext));
    c[str_size] = '\0';
    return c;
}

/********************************************************************//**
** Opens pictDB file and calls do_list command.
************************************************************************/
int do_list_cmd(int args, char *argv[])
{
    if (args < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        const char* db_filename = argv[1];
        TEST_FILENAME(db_filename);
        struct pictdb_file db_file;
        int errorCode = 0;
        errorCode = do_open(db_filename, "rb", &db_file);
        if(errorCode != 0) {
            return errorCode;
        }
        do_list(&db_file, STDOUT);
        do_close(&db_file);
        return errorCode;
    }
}

/********************************************************************//**
** Prepares and calls do_create command.
********************************************************************** */
int do_create_cmd(int args, char *argv[])
{
    if (args < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        next_arg(&args, &argv); // Skip command.

        const char* db_filename = argv[0];
        TEST_FILENAME(db_filename);

        uint32_t max_files = DEFAULT_MAX_FILES;
        uint16_t thumb_resX = DEFAULT_THUMB;
        uint16_t thumb_resY = DEFAULT_THUMB;
        uint16_t small_resX = DEFAULT_SMALL;
        uint16_t small_resY = DEFAULT_SMALL;

        // Look for optional arguments.
        //atouint16/32 can return 0 if an error occurs and
        //ERRNO is set to ERANGE. But here we don't want the value 0
        //so we don't need to test ERRNO when the result is 0
        while(args > 1) {
            next_arg(&args, &argv);
            if(!strcmp("-max_files", argv[0])) {
                if(args < 2) {
                    return ERR_NOT_ENOUGH_ARGUMENTS;
                } else {
                    next_arg(&args, &argv);
                    max_files = atouint32(argv[0]);
                    if(max_files > MAX_MAX_FILES || max_files == 0) {
                        return ERR_MAX_FILES;
                    }
                }
            } else if(!strcmp("-thumb_res", argv[0])) {
                if(args < 3) {
                    return ERR_NOT_ENOUGH_ARGUMENTS;
                } else {
                    next_arg(&args, &argv);
                    thumb_resX = atouint16(argv[0]);
                    if(thumb_resX == 0 || thumb_resX > MAX_THUMB) {
                        return ERR_RESOLUTIONS;
                    }
                    next_arg(&args, &argv);
                    thumb_resY = atouint16(argv[0]);
                    if(thumb_resY == 0 || thumb_resY > MAX_THUMB) {
                        return ERR_RESOLUTIONS;
                    }
                }
            } else if(!strcmp("-small_res", argv[0])) {
                if(args < 3) {
                    return ERR_NOT_ENOUGH_ARGUMENTS;
                } else {
                    next_arg(&args, &argv);
                    small_resX = atouint16(argv[0]);
                    if(small_resX == 0 || small_resX > MAX_SMALL  || small_resX < thumb_resX) {
                        return ERR_RESOLUTIONS;
                    }
                    next_arg(&args, &argv);
                    small_resY = atouint16(argv[0]);
                    if(small_resY == 0 || small_resY > MAX_SMALL || small_resY < thumb_resY) {
                        return ERR_RESOLUTIONS;
                    }
                }
            } else {
                return ERR_INVALID_ARGUMENT;
            }
        }

        // Create the file with given values or default.
        struct pictdb_file db_file;
        db_file.header.max_files = max_files;
        db_file.header.res_resized[DIM_X_THUMB] = thumb_resX;
        db_file.header.res_resized[DIM_Y_THUMB] = thumb_resY;
        db_file.header.res_resized[DIM_X_SMALL] = small_resX;
        db_file.header.res_resized[DIM_Y_SMALL] = small_resY;

        puts("Create");
        int errorCode = 0; //0 means no error
        errorCode = do_create(db_filename, &db_file);
        if(errorCode != 0) {
            return errorCode;
        }
        printf("%d item(s) written\n", db_file.header.max_files + 1);
        print_header(&db_file.header);
        do_close(&db_file);
        return errorCode;
    }
}

/********************************************************************//**
** Displays some explanations.
*********************************************************************** */
int help(int args, char *argv[])
{
    puts("pictDBM [COMMAND] [ARGUMENTS]");
    puts("  help: displays this help.");
    puts("  list <dbfilename>: list pictDB content.");
    puts("  create <dbfilename>: create a new pictDB.");
    puts("      options are:");
    puts("          -max_files <MAX_FILES>: maximum number of files.");
    puts("                                  default value is 10");
    puts("                                  maximum value is 100000");
    puts("          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.");
    puts("                                      default value is 64x64");
    puts("                                      maximum value is 128x128");
    puts("          -small_res <X_RES> <Y_RES>: resolution for small images.");
    puts("                                      default value is 256x256");
    puts("                                      maximum value is 512x512");
    puts("  read <dbfilename> <pictID> [original|orig|thumbnail|thumb|small]:");
    puts("      read an image from the pictDB and save it to a file.");
    puts("      default resolution is \"original\".");
    puts("  insert <dbfilename> <pictID> <filename>: insert a new image in the pictDB.");
    puts("  delete <dbfilename> <pictID>: delete picture pictID from pictDB.");
    puts("  gc <dbfilename> <tmp dbfilename>: performs garbage collecting on pictDB. Requires a temporary filename for copying the pictDB.");
    return 0;
}

/********************************************************************//**
** Deletes a picture from the database.
************************************************************************/
int do_delete_cmd(int args, char *argv[])
{
    if (args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        const char* pictID = argv[2];
        size_t pictIDSize = strlen(pictID);
        if(pictIDSize > MAX_PIC_ID || pictIDSize == 0) {
            return ERR_INVALID_PICID;
        }

        const char* db_filename = argv[1];
        TEST_FILENAME(db_filename);

        struct pictdb_file db_file;
        int errorCode = 0; //0 means no error
        errorCode = do_open(db_filename, "rb+", &db_file);
        if(errorCode != 0) {
            return errorCode;
        }

        errorCode = do_delete(pictID, &db_file);
        if(errorCode != 0) {
            do_close(&db_file);
            return errorCode;
        }
        do_close(&db_file);
        return errorCode;
    }
}

/********************************************************************//**
** Reads a picture from the database and save it in a file .jpg
************************************************************************/
int do_read_cmd(int args, char *argv[])
{
    if (args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        const char* db_filename = argv[1];
        TEST_FILENAME(db_filename);

        const char* pictID = argv[2];
        size_t pictIDSize = strlen(pictID);
        if(pictIDSize > MAX_PIC_ID || pictIDSize == 0) {
            return ERR_INVALID_PICID;
        }
        size_t dim = 0;
        if(args  == 4) {
            int dim_ = resolution_atoi(argv[3]);
            if(-1 == dim_) {
                return ERR_INVALID_ARGUMENT;
            }
            dim = (size_t) dim_;
        } else {
            dim = RES_ORIG;
        }

        struct pictdb_file db_file;
        int errorCode = 0; //0 means no error
        errorCode = do_open(db_filename, "rb+", &db_file);
        if(errorCode != 0) {
            return errorCode;
        }

        char* image_buffer = NULL;
        uint32_t image_size = 0;
        errorCode = do_read(pictID, dim, &image_buffer, &image_size, &db_file);
        if(0 != errorCode) {
            do_close(&db_file);
            return errorCode;
        }
        const char* name = create_name(pictID, dim);

        FILE* image = NULL;
        image = fopen(name, "wb");
        if(NULL == image) {
            free((void*)name);
            free(image_buffer);
            image_buffer = NULL;
            do_close(&db_file);
            return ERR_IO;
        }
        errorCode = write_disk_image(image_buffer, image_size, image);
        fclose(image);
        free((void*)name);
        free(image_buffer);
        image_buffer = NULL;
        do_close(&db_file);
        if(0 != errorCode) {
            return ERR_IO;
        }
        return 0;
    }
}


/********************************************************************//**
** Remove all deleted image from the database file.
************************************************************************/
int do_gc_cmd(int args, char *argv[])
{
    if(args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        const char* db_filename = argv[1];
        TEST_FILENAME(db_filename);

        const char* tmpdb_filename = argv[2];
        TEST_FILENAME(tmpdb_filename);

        struct pictdb_file db_file;
        int errorCode = 0; //0 means no error
        errorCode = do_open(db_filename, "rb+", &db_file);
        if(errorCode != 0) {
            return errorCode;
        }

        errorCode = do_gbcollect(&db_file, db_filename, tmpdb_filename);
        do_close(&db_file);
        return errorCode;
    }
}

/********************************************************************//**
** Insert a picture in the database.
************************************************************************/
int do_insert_cmd(int args, char *argv[])
{
    if(args < 4) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        const char* db_filename = argv[1];
        TEST_FILENAME(db_filename);

        const char* image_name = argv[3];
        TEST_FILENAME(image_name);

        const char* pictID = argv[2];
        size_t pictIDSize = strlen(pictID);
        if(pictIDSize > MAX_PIC_ID || pictIDSize == 0) {
            return ERR_INVALID_PICID;
        }

        int err_code = 0;
        long size = 0;

        char* image = NULL;
        FILE* im_file = NULL;
        im_file = fopen(image_name, "rb");
        if(NULL == im_file) {
            return ERR_IO;
        }

        // Get the size of the image in bytes
        err_code = get_image_size(im_file, &size);
        if(0 != err_code) {
            return err_code;
        }

        // Reads and stores it.
        err_code = read_disk_image(&image, size, im_file);
        if(0 != err_code) {
            return err_code;
        }
        fclose(im_file);

        // Inserts the image in the database "db_filename".
        struct pictdb_file db_file;
        err_code = do_open(db_filename, "rb+", &db_file);
        if(0 != err_code) {
            free(image);
            image = NULL;
            return err_code;
        }

        if(db_file.header.num_files >= db_file.header.max_files) {
            free(image);
            image = NULL;
            do_close(&db_file);
            return ERR_FULL_DATABASE;
        }

        err_code = do_insert(image, size, pictID, &db_file);
        free(image);
        image = NULL;
        do_close(&db_file);
        return err_code;
    }
}

/********************************************************************//**
** MAIN
************************************************************************/
int main(int args, char* argv[])
{
    command_mapping commands[N_COMMANDS] = {
        {"list", do_list_cmd},
        {"create", do_create_cmd},
        {"help", help},
        {"delete", do_delete_cmd},
        {"insert", do_insert_cmd},
        {"read", do_read_cmd},
        {"gc", do_gc_cmd}
    };

    int ret = 0;

    if(VIPS_INIT(argv[0])) {
        ret = ERR_VIPS;
    } else if (args < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        next_arg(&args, &argv); // skips command call name
        size_t i = 0;
        while(strcmp(commands[i].name, argv[0]) != 0 && i < N_COMMANDS) {
            ++i;
        }
        if(i < N_COMMANDS) {
            ret = commands[i].cmd(args, argv);
        } else {
            ret = ERR_INVALID_COMMAND;
        }
    }

    if(ret) {
        fprintf(stderr, "ERROR: %s\n", ERROR_MESSAGES[ret]);
        (void)help(0, NULL);
    }

    vips_shutdown();
    return ret;
}
