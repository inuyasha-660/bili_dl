#include "utils/utils.h"
#include "api/api.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_file_exists(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        fclose(file);
        return 1;
    } else {
        return 0;
    }
}

int is_dir_exist(char *dir)
{
    DIR *ret = opendir(dir);
    if (ret == NULL) {
        return 0;
    }
    closedir(ret);
    return 1;
}

char *int_to_str(long num)
{
    int len = snprintf(NULL, 0, "%ld", num);
    char *ret = (char *)malloc((len + 1) * sizeof(char));
    if (ret == NULL) {
        return NULL;
    }
    snprintf(ret, len + 1, "%ld", num);

    return ret;
}

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Fail to open %s\n", filename);
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    char *ret = (char *)malloc((size + 1) * sizeof(char));
    fseek(file, 0L, SEEK_SET);

    int inx = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        ret[inx] = ch;
        inx++;
    }
    ret[inx] = '\0';
    fclose(file);

    return ret;
}

size_t api_curl_finish(void *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t len_buffer = size * nmemb;
    Buffer *buffer_s = (Buffer *)userp;
    buffer_s->buffer = (char *)realloc(
        buffer_s->buffer, (buffer_s->length + len_buffer + 1) * sizeof(char));

    memcpy(buffer_s->buffer + buffer_s->length, buffer, len_buffer);
    buffer_s->length += len_buffer;
    buffer_s->buffer[buffer_s->length] = '\0';

    return len_buffer;
}