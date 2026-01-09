#include "api/api.h"
#include "cJSON.h"
#include "utils/utils.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 获取番剧信息，后接id(需去除 "ss"/"md" 前缀)
const char *API_ANIME_GET_INFO =
    "https://api.bilibili.com/pgc/web/season/section?season_id=";

extern struct Account *account;
extern struct Anime *anime;
struct AnimeList *anime_l;

int api_anime_get_cid(Buffer *buffer_info)
{
    int err = 0;
    cJSON *root = cJSON_Parse(buffer_info->buffer);
    if (root == NULL) {
        err = PARSE;
        error("Failed to parse(root)");
        goto end;
    }
    cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "result");
    if (result == NULL) {
        err = PARSE;
        error("Failed to get result");
        goto end;
    }
    cJSON *main_section =
        cJSON_GetObjectItemCaseSensitive(result, "main_section");
    if (main_section == NULL) {
        err = PARSE;
        error("Failed to get main_section");
        goto end;
    }
    cJSON *episodes =
        cJSON_GetObjectItemCaseSensitive(main_section, "episodes");
    if (episodes == NULL) {
        err = PARSE;
        error("Failed to get episodes");
        goto end;
    }

    cJSON *episode_i;
    int index = 0;
    int idx_c = 0; // 引索目标剧集
    int size_l = cJSON_GetArraySize(episodes);
    anime_l->cids = (int *)malloc((size_l + 1) * sizeof(int));
    anime_l->title = (char **)malloc(size_l * sizeof(char *));
    cJSON_ArrayForEach(episode_i, episodes)
    {
        cJSON *cid = cJSON_GetObjectItemCaseSensitive(episode_i, "cid");
        cJSON *long_title =
            cJSON_GetObjectItemCaseSensitive(episode_i, "long_title");
        if (cid == NULL || long_title == NULL) {
            err = PARSE;
            error("cid || long_title is NULL(%d)", index);
            goto end;
        }
        if (!cJSON_IsNumber(cid) || !cJSON_IsString(long_title)) {
            err = PARSE;
            error("cid || long_title type error");
            goto end;
        }

        if (anime->part[0] != 0 && anime->part[idx_c] != (index + 1)) {
            index++;
            continue;
        }

        anime_l->cids[idx_c] = cid->valueint;
        anime_l->title[idx_c] = strdup(long_title->valuestring);

        index++;
        idx_c++;
    }
    anime_l->cids[idx_c] = END_P;
    anime_l->count = idx_c;

end:
    cJSON_Delete(root);

    return err;
}

int api_anime_get_info()
{
    int err = 0;
    CURL *curl = curl_easy_init();
    if (curl) {
        CURLcode err_curl;

        size_t len = snprintf(NULL, 0, "%s%s", API_ANIME_GET_INFO, anime->id);
        char *url_info = (char *)malloc((len + 1) * sizeof(char));
        snprintf(url_info, len + 1, "%s%s", API_ANIME_GET_INFO, anime->id);

        Buffer *buffer_info = malloc(sizeof(Buffer));
        buffer_info->buffer = NULL;
        buffer_info->length = 0;

        curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
        curl_easy_setopt(curl, CURLOPT_URL, url_info);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_curl_finish);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer_info);
        err_curl = curl_easy_perform(curl);
        if (err_curl != CURLE_OK) {
            err = DLE_REQ;
            goto end;
        }

        err = api_anime_get_cid(buffer_info);

    end:
        curl_easy_cleanup(curl);
        free(buffer_info->buffer);
        free(buffer_info);
        free(url_info);
    } else {
        error("Failed to initialize curl");
        err = INE;
    }

    return err;
}

int api_anime_init()
{
    int err = 0;
    anime_l = malloc(sizeof(struct AnimeList));
    anime_l->count = 0;
    anime_l->cids = NULL;
    anime_l->title = NULL;

    err = api_anime_get_info();
    if (err != 0) {
        goto end;
    }

    for (int i = 0; i < anime_l->count; i++) {
    }

end:
    return err;
}