#include <stddef.h>

int api_dl_video_init();

typedef struct {
    char *buffer;
    size_t length;
} Buffer;

extern const char *API_VIDEO_PART;
extern const char *API_VIDEO_STREAM;