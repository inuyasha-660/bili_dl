#include <stddef.h>

int api_dl_video_init();
int api_video_merge(char *filename_video, char *filename_audio, char *outdir, char *outname);

typedef struct {
    char *buffer;
    size_t length;
} Buffer;

extern const char *API_VIDEO_PART;
extern const char *API_VIDEO_STREAM;