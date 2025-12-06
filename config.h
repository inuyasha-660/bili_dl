int cfg_read();

struct Account {
    char *config_path;
    char *SESSDATA;
    char *cookie; // 格式化完成的 SESSDATA
    int MaxThread;
    int Type; // 0: 初始化 1: 视频下载
    char *Output;
    struct Video *video;
};

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