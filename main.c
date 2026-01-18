#include "api/api.h"
#include "metadata.h"
#include "utils/utils.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct Account *account;

typedef int (*read_config)(void);
typedef int (*downloader_init)(void);

struct PerformModule {
    int             type;
    read_config     read_config;
    downloader_init downloader_init;
};

struct PerformModule modules[] = {
    {TYPE_BVID, cfg_read_video, api_dl_video_init},
    {TYPE_FOLDER, cfg_read_folder, api_dl_folder_init},
    {TYPE_ANIME, cfg_read_anime, api_anime_init},
    {TYPE_END, NULL, NULL}};

int init()
{
    account = malloc(sizeof(struct Account));
    account->config_path = NULL;
    account->config_str = NULL;
    account->SESSDATA = NULL;
    account->cookie = NULL;
    account->MaxThread = 1;
    account->Type = 0;
    account->Output = NULL;

    int err_init_curl = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (err_init_curl != 0) {
        error("Failed to initialize curl: %s",
              curl_easy_strerror(err_init_curl));
        return err_init_curl;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        error("Incorrect number of arguments: %d", argc - 1);
        return ERR_INPUT;
    }
    printf("bili_dl[%s]\n", VERSION);

    // 初始化 Curl
    info("Initialize and read configuration");
    if (init() != 0) {
        error("Aborted due to an initialization error");
        return ERR_INIT;
    }
    account->config_path = strdup(argv[1]);

    // 读取通用配置
    if (cfg_read_global() != 0) {
        error("Aborted due to a configuration reading error");
        return ERR_PARSE;
    }

    printf("\nIN: %s OUT: %s\n", argv[1], account->Output);
    printf("Start performing(Y/n): ");
    char input_perform = fgetwc(stdin);
    if (input_perform != 'y' && input_perform != 'Y') {
        info("Exiting with code 0");
        return 0;
    }

    int                   err_perform = 0;
    struct PerformModule *target_mod = NULL;
    for (struct PerformModule *mod = modules; mod->type != TYPE_END; mod++) {
        if (mod->type == account->Type) {
            target_mod = mod;
        }
    }
    if (target_mod == NULL) {
        error("Invalid download type: %d", account->Type);
        return ERR_TYPE;
    }

    // 读取指定下载模式配置
    err_perform = target_mod->read_config();
    if (err_perform != 0) {
        error("Aborted with error code: %d", err_perform);
        return err_perform;
    }
    // 启动下载任务
    err_perform = target_mod->downloader_init();
    if (err_perform != 0) {
        error("Aborted with error code: %d", err_perform);
        return err_perform;
    }

    return err_perform;
}
