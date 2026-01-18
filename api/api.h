#include <stddef.h>
#include <cJSON.h>

// 哨兵值
#define END_P -1

// 运行模式
#define TYPE_BVID 1
#define TYPE_FOLDER 2
#define TYPE_ANIME 3
#define TYPE_END -1

#define _USERAGENT                                                             \
    "Mozilla/5.0 (X11; Linux x86_64; rv:146.0) Gecko/20100101 Firefox/146.0"
#define _REFERER "https://www.bilibili.com"

// 请求缓存
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

// 视频分 P
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


// 下载初始化
int api_dl_video_init();
int api_dl_folder_init();
int api_anime_init();

// 发送请求
int api_dl_file(char *url, char *filename);
Buffer * api_get_folder_ctn_json();
int api_get_wbi_key();

// 进度打印
void progress_print(int index, int total);

// 读取配置
int cfg_read_video();
int cfg_read_videolist(cJSON *VideoObjIn);
int cfg_read_folder();
int cfg_read_anime();

// 合并视频
int api_video_merge(char *filename_video, char *filename_audio, char *outdir, char *outname,  char *outcid);


extern const char *API_LOGIN_INFO_NAV;
extern const char *API_VIDEO_PART;
extern const char *API_VIDEO_STREAM;
extern const char *API_FOLDER_CTN;
extern const char *API_ANIME_GET_STREAM;