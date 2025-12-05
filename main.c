#include "api/api.h"
#include "config.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define VERSION "0.1.1"

struct Account *account;

int init()
{
    account = malloc(sizeof(struct Account));
    account->config_path = NULL;
    account->SESSDATA = NULL;
    account->cookie = NULL;
    account->MaxThread = 1;
    account->Type = 0;
    account->Outoput = NULL;
    account->video = NULL;

    int curl_err = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curl_err != 0) {
        fprintf(stderr, "Error: Fail to initialize curl: %s\n", curl_easy_strerror(curl_err));
        return curl_err;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Error: Wrong number of arguments(%d)\n", argc - 1);
        return 1;
    }
    printf("bili_dl[%s]\n", VERSION);

    printf("INFO: Initialize and read configuration\n");
    if (init() != 0) {
        fprintf(stderr, "Exit with initialization error\n");
        return 1;
    }
    account->config_path = strdup(argv[1]);

    if (cfg_read() != 0) {
        fprintf(stderr, "Exit with reading configuration error\n");
        return 1;
    }

    printf("\nIN: %s OUT: %s\n", argv[1], account->Outoput);
    printf("Start performing(Y/n): ");
    char confirm = fgetwc(stdin);
    if (confirm != 'y' && confirm != 'Y') {
        fprintf(stdout, "Exit with 0\n");
        return 0;
    }

    int err_perform = 0;
    switch (account->Type) {
    case 1: {
        err_perform = api_dl_video_init();
        break;
    }
    default: {
        fprintf(stderr, "Error: Invalid type: %d\n", account->Type);
        return 1;
    }
    }

    return err_perform;
}
