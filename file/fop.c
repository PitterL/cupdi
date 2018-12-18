#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

/* This routine returns the size of the file it is called with. */
/*static unsigned get_file_size(const char * file_name)
{
    struct stat sb;
    if (stat(file_name, &sb) != 0) {
        fprintf(stderr, "'stat' failed for '%s': %s.\n",
            file_name, strerror(errno));
        return 0;
    }
    return sb.st_size;
}*/
unsigned long get_file_size(const char* file)
{
    FILE * f = fopen(file, "r");
    fseek(f, 0, SEEK_END);
    unsigned long len = (unsigned long)ftell(f);
    fclose(f);
    return len;
}

/*
This routine reads the entire file into memory. user should release the memory after use.
@file_name: file name
@return file content string, NULL means failed
*/
unsigned char *read_whole_file(const char * file_name)
{
    unsigned s;
    unsigned char * contents = NULL;
    FILE * f = NULL;
    size_t bytes_read;
    int status;

    s = get_file_size(file_name);
    if (!s) {
        fprintf(stderr, "File '%s' not exist or Null\n", file_name);
        return NULL;
    }

    contents = malloc(s + 1);
    if (!contents) {
        fprintf(stderr, "Not enough memory.\n");
        return NULL;
    }

    f = fopen(file_name, "rb");
    if (!f) {
        fprintf(stderr, "Could not open '%s': %s.\n", file_name,
            strerror(errno));
        goto failed;
    }
    bytes_read = fread(contents, sizeof(unsigned char), s, f);
    if (bytes_read != s) {
        fprintf(stderr, "Short read of '%s': expected %d bytes "
            "but got %d: %s.\n", file_name, s, bytes_read,
            strerror(errno));
        /*Fixme: ftell() size may larger than fread()*/
        //goto failed;
    }
    status = fclose(f);
    if (status != 0) {
        fprintf(stderr, "Error closing '%s': %s.\n", file_name,
            strerror(errno));
        goto failed;
    }
    return contents;

failed:
    if (contents)
        free(contents);

    if (f)
        fclose(f);

    return NULL;
}


/*
Combine the input <main name> + <ext name>, user should release the memory after use.
@name: original name
@extname: change the extname
@return new combined name, NULL means failed
*/
char *make_name_with_extesion(const char *name, const char *extname)
{
    char *new_name;
    int i, size, mainsize, extsize, new_size;

    if (!name || !extname) {
        fprintf(stderr, "Name or Extname is Null\n");
        return NULL;
    }

    size = strlen(name) + 1;
    extsize = strlen(extname);
    //search '.' position
    //  last character is NULL, and EXT name could get 3 characters max, so [-2: -4] is ext name, and [-5] is '.'
    for (i = -5; i <= -2; i++) {
        if (name[size + i] == '.')
            break;
    }

    mainsize = size + i;
    new_size = mainsize + extsize + 2; /*<main>.<ext>[NULL]*/
    new_name = malloc(new_size);
    if (!new_name) {
        fprintf(stderr, "Not enough memory.\n");
        return NULL;
    }

    strncpy(new_name, name, mainsize);
    new_name[mainsize] = '.';
    strncpy(new_name + mainsize + 1, extname, extsize + 1); //Copy 'NULL'

    return new_name;
}