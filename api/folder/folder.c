#include "api/api.h"
#include "utils/utils.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>

extern struct Folder *folder;
extern struct Account *account;
extern struct Video *video_s;

// 获取收藏夹全部内容
const char *API_FOLDER_CTN =
    "https://api.bilibili.com/x/v3/fav/resource/ids?media_id=";

Buffer *api_get_folder_ctn_json()
{
    Buffer *buffer_ctn = malloc(sizeof(Buffer));
    buffer_ctn->buffer = NULL;
    buffer_ctn->length = 0;

    CURL *curl = curl_easy_init();
    if (curl) {
        CURLcode err_ctn;
        size_t len = snprintf(NULL, 0, "%s%s", API_FOLDER_CTN, folder->fid);
        char *url = (char *)malloc((len + 1) * sizeof(char));
        snprintf(url, len + 1, "%s%s", API_FOLDER_CTN, folder->fid);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_curl_finish);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer_ctn);

        err_ctn = curl_easy_perform(curl);
        if (err_ctn != CURLE_OK) {
            error("Failed to get %s", url);
        }

        free(url);
        curl_easy_cleanup(curl);

    } else {
        error("Failed to initiliza curl");
    }

    return buffer_ctn;
}

int api_dl_folder_init() { return api_dl_video_init(); }
