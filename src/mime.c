#include "../include/mime.h"
#include "../include/http.h"
#include "../include/files.h"
#include "../include/mime.h"
#include <string.h>

typedef struct {
    const char *extension;
    const char *mime_type;
} mime_entry_t;

static const mime_entry_t mime_types[] = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".txt", "text/plain"},
    {NULL, "application/octet-stream"}
};

const char* mime_get_type(const char *filename){
    if (!filename) return "application/octet-stream";

    const char *dot = strrchr(filename, '.');
    if (!dot) return "application/octet-stream";
    for (int i = 0; mime_types[i].extension; i++){
        if (strcmp(mime_types[i].extension, dot) == 0){
            return mime_types[i].mime_type;
        }
    }

    return "application/octet-stream";
}

  


