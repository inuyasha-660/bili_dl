#include <stddef.h>
#include <cJSON.h>

#define P_ALL 0 // 0: 选中所有分P

enum {
    ERR_INPUT = 1, // 输入
    ERR_INIT = 2, // 初始化
    ERR_PARSE  = 3, // 解析
    ERR_REQ = 4, // 请求
    ERR_FOP = 5, // 文件操作
    ERR_TYPE = 6, // 数据类型
    ERR_CALL = 7, // 函数调用
};


// 读取通用配置文件
int cfg_read_global();

// 文件&文件夹检查
int is_file_exists(const char *filename);
int is_dir_exist(char *dir);
// 创建文件夹
int create_outdir(char *dirname);

// 读取文件
char *read_file(const char *filename);

// 数字转换
char *int_to_str(long num);

// 请求回调
size_t api_curl_finish(void *buffer, size_t size, size_t nmemb, void *userp);


// 日志等级
enum {
    LOG_INFO = 0,
    LOG_WARN = 1,
    LOG_ERR = 2,
};

void _log(int log_level, const char *func, char *format, ...);
#define info(...)  _log(LOG_INFO, __FUNCTION__, ##__VA_ARGS__)
#define warn(...)  _log(LOG_WARN, __FUNCTION__, ##__VA_ARGS__)
#define error(...)  _log(LOG_ERR, __FUNCTION__, ##__VA_ARGS__)

/*
  struct Account:

  Type = 1: 通过 Bvid 下载视频
  Type = 2: 下载收藏家视频
  Type = 3: 下载番剧
*/
struct Account {
    char *config_path;
    char *config_str;
    char *SESSDATA;
    char *cookie;
    int MaxThread;
    int Type;
    char *Output;
};

struct Wbi {
    char *img_key;
    char *sub_key;
};
