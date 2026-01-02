#include <stddef.h>
#include <cJSON.h>
#define END_P -1

typedef struct {
    char *buffer;
    size_t length;
} Buffer;

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

int api_dl_video_init();
int cfg_read_video(cJSON *VideoObjIn);
int api_video_merge(char *filename_video, char *filename_audio, char *outdir, char *outname,  char *outcid);
Buffer * api_get_folder_ctn_json();
int cfg_read_folder();
int api_dl_folder_init();
int cfg_read_anime();
int api_get_wbi_key();

extern const char *API_LOGIN_INFO_NAV;
extern const char *API_VIDEO_PART;
extern const char *API_VIDEO_STREAM;
extern const char *API_FOLDER_CTN;
