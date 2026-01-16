#include "api/api.h"
#include "metadata.h"
#include "utils/utils.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct Account *account;

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

    int curl_err = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curl_err != 0) {
        error("Fail to initialize curl: %s", curl_easy_strerror(curl_err));
        return curl_err;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        error("Error: Wrong number of arguments(%d)\n", argc - 1);
        return ERR_INPUT;
    }
    printf("bili_dl[%s]\n", VERSION);

    info("Initialize and read configuration");
    if (init() != 0) {
        error("Exit with initialization error");
        return ERR_INIT;
    }
    account->config_path = strdup(argv[1]);

    if (cfg_read_global() != 0) {
        error("Exit with reading configuration error");
        return ERR_PARSE;
    }

    printf("\nIN: %s OUT: %s\n", argv[1], account->Output);
    printf("Start performing(Y/n): ");
    char confirm = fgetwc(stdin);
    if (confirm != 'y' && confirm != 'Y') {
        info("Exit with code 0");
        return 0;
    }

    int err_perform = 0;
    switch (account->Type) {
    case 1: {
        if (cfg_read_video(NULL) != 0) {
            error("Failed to read %s", argv[1]);
            break;
        }
        err_perform = api_dl_video_init();
        break;
    }
    case 2: {
        if (cfg_read_folder() != 0) {
            error("Failed to read %s", argv[1]);
            break;
        }
        err_perform = api_dl_folder_init();
        break;
    }
    case 3: {
        if (cfg_read_anime() != 0) {
            error("Failed to read %s", argv[1]);
            break;
        }
        err_perform = api_anime_init();
        break;
    }
    default: {
        error("Invalid type: %d", account->Type);
        return ERR_TYPE;
    }
    }

    return err_perform;
}
