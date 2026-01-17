#include "api/api.h"
#include "cJSON.h"
#include "libs/thpool.h"
#include "utils/utils.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <iso646.h>
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

/*
  'accept_quality' 数组包含所有支持的画质，倒序排列
  'dash->video' 数组包含所有可获得的视频流，每个画质一般有两种编码，通过
  'dash->video->backupUrl->codecs' 标明；

  'accept_quality' 中的画质引索和 'dash-video' 中的相对应，如 accept_quality[0]
  = 120，则 'dash->video[0/1]' 为 qn = 120 的不同编码的视频流
*/
int api_anime_parse_stream(Buffer *buffer_stream, int idx_c)
{
    int err = 0;
    pthread_mutex_lock(&lock_gl);
    int mode = anime->mode;
    char *outdir = strdup(account->Output);
    int cid = anime_list->cids[idx_c];
    char *target_qa = strdup(anime->qn);
    char *target_code = strdup(anime->coding);
    char *title = strdup(anime_list->title[idx_c]);
    pthread_mutex_unlock(&lock_gl);

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
    int idx_qa = 0;

    if (!strcmp(target_qa, "*"))
        goto get_video;

    for (idx_qa = 0; idx_qa < size_qa; idx_qa++) {
        cJSON *qa = cJSON_GetArrayItem(accept_quality, idx_qa);
        if (qa == NULL || !cJSON_IsNumber(qa)) {
            error("Failed to get qa(%d)", cid);
            err = ERR_PARSE;
            goto end;
        }
        if (qa->valueint == atoi(target_qa)) {
            idx_qa *= 2;
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

    cJSON *video;
    for (int i = 0; i < 2; i++) {
        video = cJSON_GetArrayItem(video_arr, (idx_qa + i));
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

        if (!strcmp(target_code, "*") ||
            !strcmp(target_code, codecs->valuestring)) {
            goto get_video_url;
        }
    }
    error("coding %s not found", target_code);
    err = ERR_PARSE;

    char *url_video = NULL;
get_video_url: {
    cJSON *backupUrl = cJSON_GetObjectItemCaseSensitive(video, "backupUrl");
    if (backupUrl == NULL) {
        error("Failed to get backupUrl");
        err = ERR_PARSE;
        goto end;
    }
    cJSON *url_video_j = cJSON_GetArrayItem(backupUrl, 0);
    if (url_video_j == NULL || !cJSON_IsString(url_video_j)) {
        error("Failed to get backupUrl[0]");
        err = ERR_PARSE;
        goto end;
    }
    url_video = strdup(url_video_j->valuestring);
}

    cJSON *audio_arr = cJSON_GetObjectItemCaseSensitive(dash, "audio");
    if (audio_arr == NULL) {
        error("Failed to get dash->audio");
        err = ERR_PARSE;
        goto end;
    }
    cJSON *audio = cJSON_GetArrayItem(audio_arr, 0);
    if (audio == NULL) {
        error("Failed to get dash->audio[0]");
        err = ERR_PARSE;
        goto end;
    }
    cJSON *backupUrl_audio =
        cJSON_GetObjectItemCaseSensitive(audio, "backupUrl");
    if (backupUrl_audio == NULL) {
        error("Failed to get dash->audio[0]->backupUrl");
        err = ERR_PARSE;
        goto end;
    }
    cJSON *url_audio_j = cJSON_GetArrayItem(backupUrl_audio, 0);

    size_t filename_audio_len =
        snprintf(NULL, 0, "%s/%d-%s-audio.m4s", outdir, cid, title);
    char *filename_audio_str =
        (char *)malloc((filename_audio_len + 1) * sizeof(char));
    snprintf(filename_audio_str, filename_audio_len + 1, "%s/%d-%s-audio.m4s",
             outdir, cid, title);

    size_t filename_video_len =
        snprintf(NULL, 0, "%s/%d-%s-video.m4s", outdir, cid, title);
    char *filename_video_str =
        (char *)malloc((filename_video_len + 1) * sizeof(char));
    snprintf(filename_video_str, filename_video_len + 1, "%s/%d-%s-video.m4s",
             outdir, cid, title);

    size_t filename_out_len =
        snprintf(NULL, 0, "%s/%d-%s.m4s", outdir, cid, title);
    char *filename_out_str =
        (char *)malloc((filename_out_len + 1) * sizeof(char));
    snprintf(filename_out_str, filename_out_len + 1, "%s/%d-%s.m4s", outdir,
             cid, title);
    char *cid_str = int_to_str(cid);

    pthread_mutex_lock(&lock_gl);
    if (create_outdir(outdir) != 0) {
        pthread_mutex_unlock(&lock_gl);
        goto end;
    }

    switch (mode) {
    case 0: {
        api_dl_file(url_video, filename_video_str);
        api_dl_file(url_audio_j->valuestring, filename_audio_str);
        api_video_merge(filename_video_str, filename_audio_str, outdir, title,
                        cid_str);

        if (remove(filename_audio_str) != 0) {
            error("Failed to remove %s", filename_audio_str);
        }
        if (remove(filename_video_str) != 0) {
            error("Failed to remove %s", filename_video_str);
        }
        break;
    }
    case 1: {
        api_dl_file(url_video, filename_video_str);
        break;
    }
    case 2: {
        api_dl_file(url_audio_j->valuestring, filename_audio_str);
        break;
    }
    default: {
        error("No match mode: %d", mode);
        break;
    }
    }

    free(url_video);
    free(filename_video_str);
    free(filename_out_str);
    free(cid_str);
    free(filename_audio_str);
}

end:
    cJSON_Delete(root);
    free(target_qa);
    free(target_code);
    free(outdir);
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
        curl_easy_setopt(curl, CURLOPT_REFERER, _REFERER);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, _USERAGENT);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_curl_finish);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer_stream);

        info("Get %s", url);
        err_req = curl_easy_perform(curl);
        if (err_req != CURLE_OK) {
            error("Failed to get json(%d): %s", cid,
                  curl_easy_strerror(err_req));
            err = err_req;
            goto end;
        }
        err = api_anime_parse_stream(buffer_stream, idx_c);

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