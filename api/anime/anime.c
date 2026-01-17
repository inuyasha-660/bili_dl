#include "api/api.h"
#include "cJSON.h"
#include "libs/thpool.h"
#include "utils/utils.h"
#include <curl/curl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 获取番剧信息，后接id(需去除 "ss"/"md" 前缀)
const char *API_ANIME_GET_INFO =
    "https://api.bilibili.com/pgc/web/season/section?season_id=";

// 获取番剧视频流
const char *API_ANIME_GET_STREAM =
    "https://api.bilibili.com/pgc/player/web/playurl";
extern struct Account *account;
extern struct Anime *anime;
struct AnimeList *anime_list;
static pthread_mutex_t lock_gl;

int api_anime_parse_stream(Buffer *buffer_stream, int cid)
{
    int err = 0;
    char *target_code = NULL;
    cJSON *root = cJSON_Parse(buffer_stream->buffer);
    if (root == NULL) {
        error("Failed to parse root(%d)", cid);
        err = ERR_PARSE;
        goto end;
    }
    cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "result");
    if (result == NULL) {
        error("Failed to parse result(%d)", cid);
        err = ERR_PARSE;
        goto end;
    }
    cJSON *accept_quality =
        cJSON_GetObjectItemCaseSensitive(result, "accept_quality");
    if (accept_quality == NULL) {
        error("Failed to parse accept_quality(%d)", cid);
        err = ERR_PARSE;
        goto end;
    }
    int size_qa = cJSON_GetArraySize(accept_quality);
    pthread_mutex_lock(&lock_gl);
    int mode = anime->mode;
    int target_qa = atoi(anime->qn);
    target_code = strdup(anime->coding);
    pthread_mutex_unlock(&lock_gl);
    int idx_qa;

    for (idx_qa = 0; idx_qa < size_qa; idx_qa++) {
        cJSON *qa = cJSON_GetArrayItem(accept_quality, idx_qa);
        if (qa == NULL || !cJSON_IsNumber(qa)) {
            error("Failed to get qa(%d)", cid);
            err = ERR_PARSE;
            goto end;
        }
        if (qa->valueint == target_qa) {
            goto get_video;
        }
    }
    error("qn %d not found", target_qa);
    err = ERR_PARSE;
    goto end;

get_video: {
    cJSON *dash = cJSON_GetObjectItemCaseSensitive(result, "dash");
    if (dash == NULL) {
        error("Failed to parse dash(%d)", cid);
        err = ERR_PARSE;
        goto end;
    }
    cJSON *video_arr = cJSON_GetObjectItemCaseSensitive(dash, "video");
    if (video_arr == NULL) {
        error("Failed to parse video(%d)", cid);
        err = ERR_PARSE;
        goto end;
    }
    int size_video = cJSON_GetArraySize(video_arr);

    /*
      若非大会员获取 DASH 格式视频流时，accept_quality 与 video
      的对象个数不相等， 需去除 4K/2K/1080P+ 等质量后获取目标视频流
    */
    int idx_video = idx_qa - (size_qa - size_video);
    cJSON *video = cJSON_GetArrayItem(video_arr, idx_video);
    if (video == NULL) {
        error("Failed to get target video object");
        err = ERR_PARSE;
        goto end;
    }
    cJSON *codecs = cJSON_GetObjectItemCaseSensitive(video, "codecs");
    if (codecs == NULL || !cJSON_IsString(codecs)) {
        error("Failed to get codecs");
        err = ERR_PARSE;
        goto end;
    }

    if (strcmp(target_code, "*") || strcmp(target_code, codecs->valuestring)) {
        error("coding %s not found", target_code);
        err = ERR_PARSE;
    }

    cJSON *backupUrl = cJSON_GetObjectItemCaseSensitive(video, "backupUrl");
    if (backupUrl == NULL) {
        error("Failed to get backupUrl");
        err = ERR_PARSE;
        goto end;
    }
    cJSON *url = cJSON_GetArrayItem(backupUrl, 0);
    if (url == NULL || !cJSON_IsString(url)) {
        error("Failed to get backupUrl[0]");
        err = ERR_PARSE;
        goto end;
    }

    // TODO: 下载实现
    switch (mode) {
    case 0: {

        break;
    }
    case 1: {
        break;
    }
    case 2: {
        break;
    }
    default: {
        error("No match mode: %d", mode);
        break;
    }
    }

    goto end;
}

end:
    cJSON_Delete(root);
    free(target_code);
    return err;
}

int api_anime_get_stream(int idx_c)
{
    int err = 0;
    CURL *curl = curl_easy_init();
    if (curl) {
        CURLcode err_req;
        Buffer *buffer_stream = malloc(sizeof(Buffer));
        buffer_stream->buffer = NULL;
        buffer_stream->length = 0;

        pthread_mutex_lock(&lock_gl);
        int cid = anime_list->cids[idx_c];
        curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
        pthread_mutex_unlock(&lock_gl);

        size_t len = snprintf(NULL, 0, "%s?cid=%d&fnval=16&fourk=1",
                              API_ANIME_GET_STREAM, cid);
        char *url = (char *)malloc((len + 1) * sizeof(char));
        snprintf(url, len + 1, "%s?cid=%d&fnval=16&fourk=1",
                 API_ANIME_GET_STREAM, cid);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_curl_finish);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer_stream);

        err_req = curl_easy_perform(curl);
        if (err_req != CURLE_OK) {
            error("Failed to get json(%d): %s", cid,
                  curl_easy_strerror(err_req));
            err = err_req;
            goto end;
        }
        err = api_anime_parse_stream(buffer_stream, cid);

    end:
        free(buffer_stream->buffer);
        free(buffer_stream);
        free(url);

    } else {
        error("Failed to create a new curl");
        err = ERR_REQ;
    }

    return err;
}

void api_anime_get_video(int idx_c) { api_anime_get_stream(idx_c); }

int api_anime_get_cid(Buffer *buffer_info)
{
    int err = 0;
    cJSON *root = cJSON_Parse(buffer_info->buffer);
    if (root == NULL) {
        err = ERR_PARSE;
        error("Failed to parse(root)");
        goto end;
    }
    cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "result");
    if (result == NULL) {
        err = ERR_PARSE;
        error("Failed to get result");
        goto end;
    }
    cJSON *main_section =
        cJSON_GetObjectItemCaseSensitive(result, "main_section");
    if (main_section == NULL) {
        err = ERR_PARSE;
        error("Failed to get main_section");
        goto end;
    }
    cJSON *episodes =
        cJSON_GetObjectItemCaseSensitive(main_section, "episodes");
    if (episodes == NULL) {
        err = ERR_PARSE;
        error("Failed to get episodes");
        goto end;
    }

    cJSON *episode_i;
    int index = 0;
    int idx_c = 0; // 引索目标剧集
    int size_l = cJSON_GetArraySize(episodes);
    anime_list->cids = (int *)malloc((size_l + 1) * sizeof(int));
    anime_list->title = (char **)malloc(size_l * sizeof(char *));
    cJSON_ArrayForEach(episode_i, episodes)
    {
        cJSON *cid = cJSON_GetObjectItemCaseSensitive(episode_i, "cid");
        cJSON *long_title =
            cJSON_GetObjectItemCaseSensitive(episode_i, "long_title");
        if (cid == NULL || long_title == NULL) {
            err = ERR_PARSE;
            error("cid || long_title is NULL(%d)", index);
            goto end;
        }
        if (!cJSON_IsNumber(cid) || !cJSON_IsString(long_title)) {
            err = ERR_PARSE;
            error("cid || long_title type error");
            goto end;
        }

        if (anime->part[0] != 0 && anime->part[idx_c] != (index + 1)) {
            index++;
            continue;
        }

        anime_list->cids[idx_c] = cid->valueint;
        anime_list->title[idx_c] = strdup(long_title->valuestring);

        index++;
        idx_c++;
    }
    anime_list->cids[idx_c] = END_P;
    anime_list->count = idx_c;

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
            error("Failed to get cid: %s", curl_easy_strerror(err_curl));
            err = ERR_REQ;
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
        err = ERR_INIT;
    }

    return err;
}

int api_anime_init()
{
    int err = 0;
    anime_list = malloc(sizeof(struct AnimeList));
    anime_list->count = 0;
    anime_list->cids = NULL;
    anime_list->title = NULL;

    err = api_anime_get_info();
    if (err != 0) {
        goto end;
    }

    threadpool thpool_dl = thpool_init(account->MaxThread);
    pthread_mutex_init(&lock_gl, NULL);
    for (int i = 0; i < anime_list->count; i++) {
        thpool_add_work(thpool_dl, (void *)api_anime_get_video,
                        (void *)(uintptr_t)i);
    }
    thpool_wait(thpool_dl);
    thpool_destroy(thpool_dl);

end:
    return err;
}