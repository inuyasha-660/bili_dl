#include "api.h"
#include "libs/thpool.h"
#include "utils/utils.h"
#include <cJSON.h>
#include <curl/curl.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define _USERAGENT                                                             \
    "Mozilla/5.0 (X11; Linux x86_64; rv:146.0) Gecko/20100101 Firefox/146.0"
#define _REFERER "https://www.bilibili.com"

extern struct Account *account;
struct Video *video;
static pthread_mutex_t lock_gl;

const char *API_VIDEO_PART = "https://api.bilibili.com/x/player/pagelist";
const char *API_VIDEO_STREAM = "https://api.bilibili.com/x/player/playurl";

char *int_to_str(long num)
{
    int len = snprintf(NULL, 0, "%ld", num);
    char *ret = (char *)malloc((len + 1) * sizeof(char));
    if (ret == NULL) {
        return NULL;
    }
    snprintf(ret, len + 1, "%ld", num);

    return ret;
}

int is_dir_exist(char *dir)
{
    DIR *ret = opendir(dir);
    if (ret == NULL) {
        return 0;
    }
    closedir(ret);
    return 1;
}

size_t api_curl_finish(void *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t len_buffer = size * nmemb;
    Buffer *buffer_s = (Buffer *)userp;
    buffer_s->buffer = (char *)realloc(
        buffer_s->buffer, (buffer_s->length + len_buffer + 1) * sizeof(char));

    memcpy(buffer_s->buffer + buffer_s->length, buffer, len_buffer);
    buffer_s->length += len_buffer;
    buffer_s->buffer[buffer_s->length] = '\0';

    return len_buffer;
}

void progress_print(int index)
{
    static int completed = 0;
    completed += 1;
    pthread_mutex_lock(&lock_gl);
    printf("\033[32m[Completed]\033[0m [%d/%d](index: %d)\n", completed,
           account->video->count, index);
    pthread_mutex_unlock(&lock_gl);
}

int api_dl_file(char *url, char *filename)
{
    CURL *curl = curl_easy_init();
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open %s\n", filename);
    }
    flockfile(file);
    curl_easy_setopt(curl, CURLOPT_REFERER, _REFERER);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, _USERAGENT);
    pthread_mutex_lock(&lock_gl);
    curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
    pthread_mutex_unlock(&lock_gl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    printf("INFO: Get: %s\n", url);
    int err_dl = curl_easy_perform(curl);
    if (err_dl != CURLE_OK) {
        fprintf(stderr, "Error: %s\n", curl_easy_strerror(err_dl));
    }
    funlockfile(file);

    fclose(file);
    curl_easy_cleanup(curl);
    return err_dl;
}

// index: account->video 引索; i: part 引索
int api_dl_video_get_file(Buffer *buffer, int index, int i, struct Part *part)
{
    int err_parse = 0;
    char *url_video = NULL;
    char *url_audio = NULL;
    cJSON *root = cJSON_Parse(buffer->buffer);
    if (root == NULL) {
        err_parse = 1;
        fprintf(stderr, "Error: Failed to parse stream json\n");
        goto end;
    }
    cJSON *code = cJSON_GetObjectItemCaseSensitive(root, "code");
    if (code == NULL || !cJSON_IsNumber(code)) {
        err_parse = 1;
        fprintf(stderr, "Error: Failed to parse code\n");
        goto end;
    }
    if (code->valueint != 0) {
        err_parse = 1;
        fprintf(stderr, "Error: Request error: %d\n", code->valueint);
        goto end;
    }

    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data == NULL) {
        err_parse = 1;
        fprintf(stderr, "Error: Failed to parse data\n");
        goto end;
    }
    cJSON *dash = cJSON_GetObjectItemCaseSensitive(data, "dash");
    if (dash == NULL) {
        err_parse = 1;
        fprintf(stderr, "Error: Failed to get data::dash\n");
        goto end;
    }
    cJSON *Videos = cJSON_GetObjectItemCaseSensitive(dash, "video");
    if (Videos == NULL) {
        err_parse = 1;
        fprintf(stderr, "Error: Failed to get dash::video\n");
        goto end;
    }
    cJSON *Audios = cJSON_GetObjectItemCaseSensitive(dash, "audio");
    if (Audios == NULL) {
        err_parse = 1;
        fprintf(stderr, "Error: Failed to get dash::audio\n");
        goto end;
    }

    pthread_mutex_lock(&lock_gl);
    char *qn_t = strdup(account->video->qn[index]);
    char *coding_t = strdup(account->video->coding[index]);
    char *audio_t = strdup(account->video->audio[index]);
    pthread_mutex_unlock(&lock_gl);

    cJSON *video;
    cJSON_ArrayForEach(video, Videos)
    {
        cJSON *id = cJSON_GetObjectItemCaseSensitive(video, "id");
        if (id == NULL || !cJSON_IsNumber(id)) {
            fprintf(stderr, "Error: id(video) is NULL\n");
            err_parse = 2;
            goto end;
        }
        char *id_str = int_to_str(id->valueint);
        cJSON *coding = cJSON_GetObjectItemCaseSensitive(video, "codecid");
        char *coding_str = int_to_str(coding->valueint);
        // 若为 "*" 则直接判断 coding
        if (id_str != NULL && !strcmp("*", qn_t)) {
            free(id_str);
            goto getCoding;
        }
        if (id_str == NULL || strcmp(id_str, qn_t)) {
            free(id_str);
            continue;
        }
        free(id_str);

    getCoding:
        if (coding_str != NULL && !strcmp("*", coding_t)) {
            free(coding_str);
            goto getbaseUrl;
        }
        if (coding == NULL || strcmp(coding_str, coding_t)) {
            free(coding_str);
            continue;
        }
        free(coding_str);

    getbaseUrl: {
        cJSON *baseUrl = cJSON_GetObjectItemCaseSensitive(video, "baseUrl");
        if (baseUrl == NULL || !cJSON_IsString(baseUrl)) {
            fprintf(stderr, "Error: baseUrl(video) is NULL\n");
            err_parse = 2;
            free(qn_t);
            free(coding_t);
            goto end;
        }
        url_video = strdup(baseUrl->valuestring);
        break;
    }
    }
    free(qn_t);
    free(coding_t);

    if (url_video == NULL) {
        pthread_mutex_lock(&lock_gl);
        fprintf(stderr, "Error: Invalid qn&coding: %s&%s\n",
                account->video->qn[index], account->video->coding[index]);
        pthread_mutex_unlock(&lock_gl);
        goto end;
    }

    cJSON *audio;
    cJSON_ArrayForEach(audio, Audios)
    {
        cJSON *id = cJSON_GetObjectItemCaseSensitive(audio, "id");
        if (id == NULL || !cJSON_IsNumber(id)) {
            fprintf(stderr, "Error: id(audio) is NULL\n");
            err_parse = 3;
            goto end;
        }
        char *id_str = int_to_str(id->valueint);
        if (id_str != NULL && !strcmp("*", audio_t)) {
            free(id_str);
            goto getbaseUrl_audio;
        }
        if (id_str == NULL || strcmp(id_str, audio_t)) {
            free(id_str);
            continue;
        }
        free(id_str);

    getbaseUrl_audio: {

        cJSON *baseUrl = cJSON_GetObjectItemCaseSensitive(audio, "baseUrl");
        if (baseUrl == NULL || !cJSON_IsString(baseUrl)) {
            fprintf(stderr, "Error: baseUrl(audio) is NULL\n");
            err_parse = 3;
            free(audio_t);
            goto end;
        }
        url_audio = strdup(baseUrl->valuestring);
        break;
    }
    }
    free(audio_t);

    if (url_audio == NULL) {
        pthread_mutex_lock(&lock_gl);
        fprintf(stderr, "Error: Invalid audio&coding: %s&%s\n",
                account->video->qn[index], account->video->coding[index]);
        pthread_mutex_unlock(&lock_gl);
        goto end;
    }

    pthread_mutex_lock(&lock_gl);
    int mode = account->video->mode[index];
    char *outdir = strdup(account->Output);
    char *outname = strdup(part->part[i]);

    if (!is_dir_exist(account->Output)) {
        int err_mk =
            mkdir(account->Output, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (err_mk != 0) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            err_parse = err_mk;
            pthread_mutex_unlock(&lock_gl);
            goto end;
        }
    }
    // 创建视频文件
    size_t len_video = snprintf(NULL, 0, "%s/%s-video.m4s", outdir, outname);
    char *filename_video = (char *)malloc((len_video + 1) * sizeof(char));
    snprintf(filename_video, len_video + 1, "%s/%s-video.m4s", outdir, outname);
    // 创建音频文件
    size_t len_audio = snprintf(NULL, 0, "%s/%s-audio.m4s", outdir, outname);
    char *filename_audio = (char *)malloc((len_audio + 1) * sizeof(char));
    snprintf(filename_audio, len_audio + 1, "%s/%s-audio.m4s", outdir, outname);
    pthread_mutex_unlock(&lock_gl);

    switch (mode) {
    case 0: { // TODO: 实现音视频合并
        api_dl_file(url_video, filename_video);
        api_dl_file(url_audio, filename_audio);
        api_video_merge(filename_video, filename_audio, outdir, outname);

        if (remove(filename_audio) != 0) {
            fprintf(stderr, "Error: Failed to remove %s\n", filename_audio);
        }
        if (remove(filename_video) != 0) {
            fprintf(stderr, "Error: Failed to remove %s\n", filename_video);
        }

        free(outdir);
        free(outname);
        free(filename_audio);
        free(filename_video);
        goto end;
    }
    case 1: {
        api_dl_file(url_video, filename_video);

        free(filename_audio);
        free(filename_video);
        free(outdir);
        free(outname);
        goto end;
    }
    case 2: {
        api_dl_file(url_audio, filename_audio);

        free(filename_audio);
        free(filename_video);
        free(outdir);
        free(outname);
        goto end;
    }
    default: {
        fprintf(stderr, "Error: Invalid value of mode\n");

        free(filename_audio);
        free(filename_video);
        free(outdir);
        free(outname);
        err_parse = 1;
        goto end;
    }
    }

end:
    if (url_audio != NULL)
        free(url_audio);
    if (url_video != NULL)
        free(url_video);
    cJSON_Delete(root);
    return err_parse;
}

int api_dl_video_get_stream_url(struct Part *part, int index)
{
    int err = 0;
    for (int i = 0; i < part->count; i++) {
        CURL *curl = curl_easy_init();
        pthread_mutex_lock(&lock_gl);
        if (account->video->part[index][0] != 0 &&
            account->video->part[index][i] != i + 1) {
            pthread_mutex_unlock(&lock_gl);
            continue;
        }

        size_t len = snprintf(NULL, 0, "%s?bvid=%s&cid=%s&fnval=16&fourk=1",
                              API_VIDEO_STREAM, account->video->Bvid[index],
                              part->cid[i]);
        char *url = (char *)malloc((len + 1) * sizeof(char));
        snprintf(url, len + 1, "%s?bvid=%s&cid=%s&fnval=16&fourk=1",
                 API_VIDEO_STREAM, account->video->Bvid[index], part->cid[i]);
        curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
        pthread_mutex_unlock(&lock_gl);

        printf("INFO: Get %s\n", url);

        if (curl) {
            CURLcode err_url;
            Buffer *buffer_u = malloc(sizeof(Buffer));
            buffer_u->buffer = NULL;
            buffer_u->length = 0;

            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer_u);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_curl_finish);
            err_url = curl_easy_perform(curl);
            if (err_url != CURLE_OK) {
                pthread_mutex_lock(&lock_gl);
                fprintf(stderr, "Error: Failed to get %s stream url\n",
                        account->video->Bvid[index]);
                pthread_mutex_unlock(&lock_gl);
                err = err_url;
                goto end;
            }

            int err_file = api_dl_video_get_file(buffer_u, index, i, part);
            if (err_file != 0) {
                pthread_mutex_lock(&lock_gl);
                fprintf(stderr, "Error: Failed to download %s\n",
                        account->video->Bvid[index]);
                pthread_mutex_unlock(&lock_gl);
                err = err_file;
            }
        end:
            curl_easy_cleanup(curl);
            free(url);
            free(buffer_u->buffer);
            free(buffer_u);

        } else {
            fprintf(stderr, "Error: Failed to create a new curl\n");
            free(url);
            return 1;
        }
    }

    return err;
}

int api_dl_video_get_cid(char *buffer, struct Part *ret)
{
    int err_get = 0;
    cJSON *root = cJSON_Parse(buffer);
    if (root == NULL) {
        fprintf(stderr, "Error: Failed to parse json::cid\n");
        err_get = -1;
        goto end;
    }
    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data == NULL) {
        fprintf(stderr, "Error: Failed to get data from cid::data\n");
        err_get = -1;
        goto end;
    }
    cJSON *parts;
    int index = 0;
    cJSON_ArrayForEach(parts, data)
    {
        cJSON *cid = cJSON_GetObjectItemCaseSensitive(parts, "cid");
        if (cid == NULL || !cJSON_IsNumber(cid)) {
            fprintf(stderr, "Error: Failed to read cid from data::cid\n");
            err_get = -1;
            goto end;
        }
        cJSON *part = cJSON_GetObjectItemCaseSensitive(parts, "part");
        if (part == NULL || !cJSON_IsString(part)) {
            fprintf(stderr, "Error: Failed to read part from data::part\n");
            err_get = -1;
            goto end;
        }

        ret->cid = (char **)realloc(ret->cid, (index + 1) * sizeof(char *));
        if (ret->cid == NULL) {
            fprintf(stderr, "Error: Failed to realloc cid array\n");
            err_get = -1;
            goto end;
        }
        ret->part = (char **)realloc(ret->part, (index + 1) * sizeof(char *));
        if (ret->part == NULL) {
            fprintf(stderr, "Error: Failed to realloc part array\n");
            err_get = -1;
            goto end;
        }

        ret->cid[index] = strdup(int_to_str((long)cid->valuedouble));
        ret->part[index] = strdup(part->valuestring);

        index += 1;
    }
    ret->count = index;

end:
    cJSON_Delete(root);
    return err_get;
}

void api_dl_video(void *index_p)
{
    pthread_mutex_lock(&lock_gl);
    int index = (uintptr_t)index_p;
    if (account->cookie == NULL) {
        fprintf(stderr, "Error: cookie is NULL\n");
        return;
    }
    pthread_mutex_unlock(&lock_gl);

    CURL *curl = curl_easy_init();
    if (curl) {
        CURLcode err_cid;
        struct Part *part = malloc(sizeof(struct Part));
        part->part = NULL;
        part->cid = NULL;
        part->count = 0;
        Buffer *buffer_cid = malloc(sizeof(Buffer));
        buffer_cid->buffer = NULL;
        buffer_cid->length = 0;

        pthread_mutex_lock(&lock_gl);
        size_t len = snprintf(NULL, 0, "%s?bvid=%s", API_VIDEO_PART,
                              account->video->Bvid[index]);
        char *url = (char *)malloc((len + 1) * sizeof(char));
        snprintf(url, len + 1, "%s?bvid=%s", API_VIDEO_PART,
                 account->video->Bvid[index]);

        curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
        pthread_mutex_unlock(&lock_gl);

        printf("INFO: Get %s\n", url);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_curl_finish);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer_cid);
        err_cid = curl_easy_perform(curl);
        if (err_cid != CURLE_OK) {
            fprintf(stderr, "%s", curl_easy_strerror(err_cid));
            goto end;
        }

        // 获取 Cid
        int err_get = api_dl_video_get_cid(buffer_cid->buffer, part);
        if (part == NULL || err_cid != 0) {
            fprintf(stderr, "Error(%d): part is NULL\n", err_get);
            goto end;
        }

        // 视频下载
        int err_dl = api_dl_video_get_stream_url(part, index);
        if (err_dl != 0) {
            fprintf(stderr, "Error: Download failed with %d\n", err_dl);
            goto end;
        }

        progress_print(index);

    end:
        free(buffer_cid->buffer);
        free(buffer_cid);
        free(part);
        free(url);
        curl_easy_cleanup(curl);

        return;
    }
    fprintf(stderr, "Error: Failed to create a new curl\n");
}

int api_dl_video_init()
{
    int err = 0;
    threadpool thpool_dl = thpool_init(account->MaxThread);
    pthread_mutex_init(&lock_gl, NULL);
    for (int i = 0; i < account->video->count; i++) {
        thpool_add_work(thpool_dl, (void *)api_dl_video, (void *)(uintptr_t)i);
    }
    thpool_wait(thpool_dl);
    thpool_destroy(thpool_dl);

    return err;
}