#include <stddef.h>
#include <cJSON.h>

#define P_ALL 0 // 0: 选中所有分P

// 程序输入错误
const int INE  = 1;
// 初始化错误
const int INITE = 2;
// 解析错误
const int PARSE  = 3;
// 请求错误
const int DLE_REQ = 4;
// 文件操作错误
const int FE_OP = 5;
// 数据类型错误
const int TYPEE = 6;
// 函数调用错误
const int CALLE = 7;

int cfg_read_global();
int is_file_exists(const char *filename);
char *read_file(const char *filename);
int is_dir_exist(char *dir);
char *int_to_str(long num);
size_t api_curl_finish(void *buffer, size_t size, size_t nmemb, void *userp);

enum {
    LOG_INFO = 0,
    LOG_WARN = 1,
    LOG_ERR = 2,
};

void _log(int log_level, const char *func, char *format, ...);
#define info(...)  _log(LOG_INFO, __FUNCTION__, ##__VA_ARGS__)
#define warn(...)  _log(LOG_WARN, __FUNCTION__, ##__VA_ARGS__)
#define error(...)  _log(LOG_ERR, __FUNCTION__, ##__VA_ARGS__)

struct Account {
    char *config_path;
    char *config_str;
    char *SESSDATA;
    char *cookie; // 格式化完成的 SESSDATA
    int MaxThread;
    int Type; // 0: 初始化 1: 视频下载
    char *Output;
};

struct Wbi {
    char *img_key;
    char *sub_key;
};
