#include "api/api.h"
#include "libs/thpool.h"
#include "utils/utils.h"
#include <cJSON.h>
#include <curl/curl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern struct Account *account;
struct Video *video_s;
static pthread_mutex_t lock_gl;

const char *API_VIDEO_PART = "https://api.bilibili.com/x/player/pagelist";
const char *API_VIDEO_STREAM = "https://api.bilibili.com/x/player/playurl";

void progress_print(int index)
{
    static int completed = 0;
    completed += 1;
    pthread_mutex_lock(&lock_gl);
    printf("\033[32m[Completed]\033[0m [%d/%d](index: %d)\n", completed,
           video_s->count, index);
    pthread_mutex_unlock(&lock_gl);
}

int api_dl_file(char *url, char *filename)
{
    CURL *curl = curl_easy_init();
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        error("Failed to open %s", filename);
        if (curl)
            curl_easy_cleanup(curl);
        return CURLE_WRITE_ERROR;
    }
    if (curl == NULL) {
        error("Failed to create a new curl");
        fclose(file);
        return CURLE_FAILED_INIT;
    }

    flockfile(file);
    curl_easy_setopt(curl, CURLOPT_REFERER, _REFERER);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, _USERAGENT);
    pthread_mutex_lock(&lock_gl);
    curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
    pthread_mutex_unlock(&lock_gl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    info("Get: %s\n", url); // 为便于查看增加一行间距
    int err_dl = curl_easy_perform(curl);
    if (err_dl != CURLE_OK) {
        error("%s", curl_easy_strerror(err_dl));
    }
    funlockfile(file);

    fclose(file);
    curl_easy_cleanup(curl);
    return err_dl;
}

int api_dl_video_get_file(Buffer *buffer, int idx_v, int idx_p,
                          struct Part *part)
{
    int err_parse = 0;
    char *url_video = NULL;
    char *url_audio = NULL;
    cJSON *root = cJSON_Parse(buffer->buffer);
    if (root == NULL) {
        err_parse = ERR_PARSE;
        error("Failed to parse stream json");
        goto end;
    }
    cJSON *code = cJSON_GetObjectItemCaseSensitive(root, "code");
    if (code == NULL || !cJSON_IsNumber(code)) {
        err_parse = ERR_PARSE;
        error("Failed to parse code");
        goto end;
    }
    if (code->valueint != 0) {
        err_parse = ERR_PARSE;
        error("Request error: %d", code->valueint);
        goto end;
    }

    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data == NULL) {
        err_parse = ERR_PARSE;
        error("Failed to parse data");
        goto end;
    }
    cJSON *dash = cJSON_GetObjectItemCaseSensitive(data, "dash");
    if (dash == NULL) {
        err_parse = ERR_PARSE;
        error("Failed to get data::dash");
        goto end;
    }
    cJSON *Videos = cJSON_GetObjectItemCaseSensitive(dash, "video");
    if (Videos == NULL) {
        err_parse = ERR_PARSE;
        error("Failed to get dash::video");
        goto end;
    }
    cJSON *Audios = cJSON_GetObjectItemCaseSensitive(dash, "audio");
    if (Audios == NULL) {
        err_parse = ERR_PARSE;
        error("Failed to get dash::audio");
        goto end;
    }

    pthread_mutex_lock(&lock_gl);
    char *qn_t = strdup(video_s->qn[idx_v]);
    char *coding_t = strdup(video_s->coding[idx_v]);
    char *audio_t = strdup(video_s->audio[idx_v]);
    pthread_mutex_unlock(&lock_gl);

    cJSON *video;
    cJSON_ArrayForEach(video, Videos)
    {
        cJSON *id = cJSON_GetObjectItemCaseSensitive(video, "id");
        if (id == NULL || !cJSON_IsNumber(id)) {
            error("id(video) is NULL");
            err_parse = ERR_PARSE;
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
            error("baseUrl(video) is NULL");
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
        error("Invalid qn&coding: %s&%s", video_s->qn[idx_v],
              video_s->coding[idx_v]);
        pthread_mutex_unlock(&lock_gl);
        goto end;
    }

    cJSON *audio;
    cJSON_ArrayForEach(audio, Audios)
    {
        cJSON *id = cJSON_GetObjectItemCaseSensitive(audio, "id");
        if (id == NULL || !cJSON_IsNumber(id)) {
            error("id(audio) is NULL");
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
            error("baseUrl(audio) is NULL");
            err_parse = ERR_PARSE;
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
        error("Invalid audio&coding: %s&%s", video_s->qn[idx_v],
              video_s->coding[idx_v]);
        pthread_mutex_unlock(&lock_gl);
        goto end;
    }

    pthread_mutex_lock(&lock_gl);
    int mode = video_s->mode[idx_v];
    char *outdir = strdup(account->Output);
    char *outname = strdup(part->part[idx_p]);
    char *outcid = strdup(part->cid[idx_p]);

    if (create_outdir(account->Output) != 0) {
        pthread_mutex_unlock(&lock_gl);
        goto end;
    }

    // if (!is_dir_exist(account->Output)) {
    //     int err_mk =
    //         mkdir(account->Output, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //     if (err_mk != 0) {
    //         char *err_s = strerror(errno);
    //         error("%s", err_s);
    //         free(err_s);
    //         err_parse = err_mk;
    //         pthread_mutex_unlock(&lock_gl);
    //         goto end;
    //     }
    // }

    // 创建视频文件
    size_t len_video =
        snprintf(NULL, 0, "%s/%s-%s-video.m4s", outdir, outcid, outname);
    char *filename_video = (char *)malloc((len_video + 1) * sizeof(char));
    snprintf(filename_video, len_video + 1, "%s/%s-%s-video.m4s", outdir,
             outcid, outname);
    // 创建音频文件
    size_t len_audio =
        snprintf(NULL, 0, "%s/%s-%s-audio.m4s", outdir, outcid, outname);
    char *filename_audio = (char *)malloc((len_audio + 1) * sizeof(char));
    snprintf(filename_audio, len_audio + 1, "%s/%s-%s-audio.m4s", outdir,
             outcid, outname);
    pthread_mutex_unlock(&lock_gl);

    switch (mode) {
    case 0: {
        api_dl_file(url_video, filename_video);
        api_dl_file(url_audio, filename_audio);
        api_video_merge(filename_video, filename_audio, outdir, outname,
                        outcid);

        if (remove(filename_audio) != 0) {
            error("Failed to remove %s", filename_audio);
        }
        if (remove(filename_video) != 0) {
            error("Failed to remove %s", filename_video);
        }

        goto free_dl;
    }
    case 1: {
        api_dl_file(url_video, filename_video);
        goto free_dl;
    }
    case 2: {
        api_dl_file(url_audio, filename_audio);
        goto free_dl;
    }
    default: {
        error("Invalid value of mode");
        err_parse = 1;
        goto free_dl;
    }
    }

free_dl: {
    free(filename_audio);
    free(filename_video);
    free(outdir);
    free(outname);
}

end:
    if (url_audio != NULL)
        free(url_audio);
    if (url_video != NULL)
        free(url_video);
    cJSON_Delete(root);
    return err_parse;
}

Buffer *api_dl_video_get_stream_url(struct Part *part, int idx_v, int idx_p)
{
    Buffer *buffer_u = malloc(sizeof(Buffer));
    buffer_u->buffer = NULL;
    buffer_u->length = 0;

    CURL *curl = curl_easy_init();
    pthread_mutex_lock(&lock_gl);

    size_t len =
        snprintf(NULL, 0, "%s?bvid=%s&cid=%s&fnval=16&fourk=1",
                 API_VIDEO_STREAM, video_s->Bvid[idx_v], part->cid[idx_p]);
    char *url = (char *)malloc((len + 1) * sizeof(char));
    snprintf(url, len + 1, "%s?bvid=%s&cid=%s&fnval=16&fourk=1",
             API_VIDEO_STREAM, video_s->Bvid[idx_v], part->cid[idx_p]);
    curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
    pthread_mutex_unlock(&lock_gl);

    info("Get %s", url);

    if (curl) {
        CURLcode err_url;

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer_u);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_curl_finish);
        err_url = curl_easy_perform(curl);
        if (err_url != CURLE_OK) {
            pthread_mutex_lock(&lock_gl);
            error("Failed to get %s stream url", video_s->Bvid[idx_v]);
            pthread_mutex_unlock(&lock_gl);

            goto end;
        }

    end:
        curl_easy_cleanup(curl);
        free(url);

    } else {
        error("Failed to create a new curl");
        free(url);
    }

    return buffer_u;
}

int api_dl_video_down(struct Part *part, int idx_v)
{
    int err = 0;
    size_t idx_p_choice = 0;
    for (int i = 0; i < part->count; i++) {
        pthread_mutex_lock(&lock_gl);
        if (video_s->part[idx_v][idx_p_choice] == -1) {
            pthread_mutex_unlock(&lock_gl);
            break;
        }

        // 存在显示分P指定且与当前分P(i)不相等时跳过
        if (video_s->part[idx_v][0] != P_ALL &&
            (i + 1) != video_s->part[idx_v][idx_p_choice]) {
            pthread_mutex_unlock(&lock_gl);
            continue;
        }
        pthread_mutex_unlock(&lock_gl);

        Buffer *buffer_u = api_dl_video_get_stream_url(part, idx_v, i);

        int err_file = api_dl_video_get_file(buffer_u, idx_v, i, part);
        if (err_file != 0) {
            pthread_mutex_lock(&lock_gl);
            error("Failed to download %s", video_s->Bvid[idx_v]);
            pthread_mutex_unlock(&lock_gl);
            err = err_file;
        }
        idx_p_choice++;
    }

    return err;
}

int api_dl_video_get_cid(char *buffer, struct Part *ret)
{
    int err_get = 0;
    cJSON *root = cJSON_Parse(buffer);
    if (root == NULL) {
        error("Failed to parse json::cid");
        err_get = ERR_PARSE;
        goto end;
    }
    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data == NULL) {
        error("Failed to get data from cid::data");
        err_get = ERR_PARSE;
        goto end;
    }
    cJSON *parts;
    int index = 0;
    cJSON_ArrayForEach(parts, data)
    {
        cJSON *cid = cJSON_GetObjectItemCaseSensitive(parts, "cid");
        if (cid == NULL || !cJSON_IsNumber(cid)) {
            error("Failed to read cid from data::cid");
            err_get = ERR_PARSE;
            goto end;
        }
        cJSON *part = cJSON_GetObjectItemCaseSensitive(parts, "part");
        if (part == NULL || !cJSON_IsString(part)) {
            error("Failed to read part from data::part");
            err_get = ERR_PARSE;
            goto end;
        }

        ret->cid = (char **)realloc(ret->cid, (index + 1) * sizeof(char *));
        if (ret->cid == NULL) {
            error("Failed to realloc cid array");
            err_get = ERR_PARSE;
            goto end;
        }
        ret->part = (char **)realloc(ret->part, (index + 1) * sizeof(char *));
        if (ret->part == NULL) {
            error("Failed to realloc part array");
            err_get = ERR_PARSE;
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
        error("cookie is NULL");
        pthread_mutex_unlock(&lock_gl);
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
                              video_s->Bvid[index]);
        char *url = (char *)malloc((len + 1) * sizeof(char));
        snprintf(url, len + 1, "%s?bvid=%s", API_VIDEO_PART,
                 video_s->Bvid[index]);

        curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
        pthread_mutex_unlock(&lock_gl);

        info("Get %s", url);

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
            error("part(%d) is NULL", err_get);
            goto end;
        }

        // 视频下载
        int err_dl = api_dl_video_down(part, index);
        if (err_dl != 0) {
            error("Download failed with %d", err_dl);
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
    error("Failed to create a new curl");
}

int api_dl_video_init()
{
    int err = 0;
    if (api_get_wbi_key() != 0) {
        err = ERR_CALL;
        return err;
    }
    threadpool thpool_dl = thpool_init(account->MaxThread);
    pthread_mutex_init(&lock_gl, NULL);
    for (int i = 0; i < video_s->count; i++) {
        thpool_add_work(thpool_dl, (void *)api_dl_video, (void *)(uintptr_t)i);
    }
    thpool_wait(thpool_dl);
    thpool_destroy(thpool_dl);

    return err;
}