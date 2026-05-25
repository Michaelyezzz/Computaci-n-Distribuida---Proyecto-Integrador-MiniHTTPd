#ifndef FILES_H
#define FILES_H

#include <stddef.h>

int file_exists(const char *path);
char* file_read(const char *path, size_t *size);
char* file_normalize_path(const char *requested_path, const char *root);

#endif