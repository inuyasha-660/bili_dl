#include <stddef.h>
#include <cJSON.h>

#define END_P -1
#define _USERAGENT                                                             \
    "Mozilla/5.0 (X11; Linux x86_64; rv:146.0) Gecko/20100101 Firefox/146.0"
#define _REFERER "https://www.bilibili.com"

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

struct Anime {
    char *id;
    int *part;
    int mode;
    char *qn;
    char *coding;
};

struct AnimeList {
    int count;
    int *cids;
    char **title;
};

int api_dl_video_init();
int cfg_read_video(cJSON *VideoObjIn);
int api_video_merge(char *filename_video, char *filename_audio, char *outdir, char *outname,  char *outcid);
Buffer * api_get_folder_ctn_json();
int cfg_read_folder();
int api_dl_folder_init();
int api_dl_file(char *url, char *filename);
int cfg_read_anime();
int api_anime_init();
int api_get_wbi_key();
int create_outdir(char *dirname);

extern const char *API_LOGIN_INFO_NAV;
extern const char *API_VIDEO_PART;
extern const char *API_VIDEO_STREAM;
extern const char *API_FOLDER_CTN;
extern const char *API_ANIME_GET_STREAM;