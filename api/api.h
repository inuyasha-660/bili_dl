#include <stddef.h>

int api_dl_video_init();
int cfg_read_video();
int api_video_merge(char *filename_video, char *filename_audio, char *outdir, char *outname);

int cfg_read_folder();

struct Video {
    int count; // 视频总数
    int *mode; // 0: 音频&视频 1: 视频 2: 音频
    int **part; // 视频分P
    char **Bvid;
    char **qn;
    char **audio;
    char **coding;
};

struct Part {
    int count;
    char **cid;
    char **part;
};

struct Folder { 
    char *fid;
    int mode;
    char *qn;
    char *audio;
    char *coding;
};

extern const char *API_VIDEO_PART;
extern const char *API_VIDEO_STREAM;