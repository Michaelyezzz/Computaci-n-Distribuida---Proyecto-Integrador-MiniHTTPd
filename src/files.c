#include "../include/files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <libgen.h>
#include <linux/limits.h>

int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

char* file_read(const char *path, size_t *size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = (char *)malloc(*size);
    if (!buffer) {
        fclose(fp);
        return NULL;

    }

    size_t read = fread(buffer, 1, *size, fp);
    fclose(fp);

    if (read != *size){
        free(buffer);
        return NULL;

    }
    return buffer;
}

char* file_normalize_path(const char *requested_path, const char *root) {
    if (strstr(requested_path, "..") != NULL) {
        return NULL;
    }

    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s%s", root, requested_path);

    return strdup(full_path);

}