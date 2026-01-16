#include "api/api.h"
#include "cJSON.h"
#include "libs/md5.h"
#include "utils.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct Wbi *wbi;
extern struct Account *account;

// 获得用户基本信息，包含 img_url、sub_url
const char *API_LOGIN_INFO_NAV = "https://api.bilibili.com/x/web-interface/nav";

const int MIXIN_KEY_ENC_TAB[64] = {
    46, 47, 18, 2,  53, 8,  23, 32, 15, 50, 10, 31, 58, 3,  45, 35,
    27, 43, 5,  49, 33, 9,  42, 19, 29, 28, 14, 39, 12, 38, 41, 13,
    37, 48, 7,  16, 24, 55, 40, 61, 26, 17, 0,  1,  60, 51, 30, 4,
    22, 25, 54, 21, 56, 59, 6,  63, 57, 62, 11, 36, 20, 34, 44, 52};

int api_wbi_padding(Buffer *buffer_wbi)
{
    int err = 0;
    wbi = malloc(sizeof(struct Wbi));
    wbi->img_key = NULL;
    wbi->sub_key = NULL;

    cJSON *root = cJSON_Parse(buffer_wbi->buffer);
    if (root == NULL) {
        error("Failed to parse root");
        err = ERR_PARSE;
        goto end;
    }
    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data == NULL) {
        error("Failed to parse data");
        err = ERR_PARSE;
        goto end;
    }
    cJSON *wbi_img = cJSON_GetObjectItemCaseSensitive(data, "wbi_img");
    if (wbi_img == NULL) {
        error("Failed to parse wbi_img");
        err = ERR_PARSE;
        goto end;
    }

    cJSON *img_url = cJSON_GetObjectItemCaseSensitive(wbi_img, "img_url");
    cJSON *sub_url = cJSON_GetObjectItemCaseSensitive(wbi_img, "img_url");
    if (img_url == NULL || sub_url == NULL) {
        error("img_url || sub_url is NULL");
        err = ERR_PARSE;
        goto end;
    }
    if (!cJSON_IsString(img_url) || !cJSON_IsString(sub_url)) {
        error("img_url || sub_url is not string");
        err = ERR_PARSE;
        goto end;
    }
    wbi->img_key = strdup(img_url->valuestring);
    wbi->sub_key = strdup(sub_url->valuestring);

end:
    cJSON_Delete(root);
    return err;
}

int api_get_wbi_key()
{
    int err = 0;
    CURL *curl = curl_easy_init();
    if (curl) {
        CURLcode code;
        Buffer *buffer_wbi = malloc(sizeof(Buffer));
        buffer_wbi->buffer = NULL;
        buffer_wbi->length = 0;

        curl_easy_setopt(curl, CURLOPT_URL, API_LOGIN_INFO_NAV);
        curl_easy_setopt(curl, CURLOPT_COOKIE, account->cookie);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_curl_finish);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer_wbi);
        code = curl_easy_perform(curl);
        if (code != CURLE_OK) {
            error("%s", curl_easy_strerror(code));
            err = code;
            goto end;
        }
        api_wbi_padding(buffer_wbi);
        free(buffer_wbi->buffer);
        free(buffer_wbi);

    } else {
        error("Failed to initialize curl");
        err = ERR_INIT;
    }

end:
    return err;
}

int cmp_args(const void *a, const void *b)
{
    char *stra = *(char **)a;
    char *strb = *(char **)b;

    return (int)stra[0] - (int)strb[0];
}

char *api_gen_wbi(const char *url_raw)
{
    if (wbi->img_key == NULL && wbi->sub_key == NULL)
        return NULL;

    size_t len = strlen(wbi->img_key) + strlen(wbi->sub_key);
    char append[len + 1];
    append[len] = '\0';
    snprintf(append, len, "%s%s", wbi->img_key, wbi->sub_key);

    char mixin_key[33];
    mixin_key[32] = '\0';
    for (size_t i = 0; i < 32; i++) {
        mixin_key[i] = append[MIXIN_KEY_ENC_TAB[i]];
    }

    time_t wts;
    time(&wts);

    char *url_bak = strdup(url_raw);
    char **list_args = NULL;
    char *url_base = strtok(url_bak, "?");
    char *arg;
    int idx = 0;
    while ((arg = strtok(NULL, "&"))) {
        list_args = (char **)realloc(list_args, (idx + 1) * sizeof(char *));
        list_args[idx] = NULL;
        list_args[idx] = strdup(arg);
        idx++;
    }

    char *wts_str = int_to_str(wts);
    size_t len_wts = strlen(wts_str);

    list_args = (char **)realloc(list_args, (idx + 1) * sizeof(char *));
    list_args[idx] = NULL;
    list_args[idx] = (char *)malloc((len_wts + 1) * sizeof(char));
    list_args[idx][len_wts] = '\0';
    sprintf(list_args[idx], "wts=%s", wts_str);
    idx++;

    qsort(list_args, idx, sizeof(char *), cmp_args);

    size_t len_ret = strlen(url_raw) + strlen(mixin_key) + len_wts + 4;
    char *ret = (char *)malloc(((len_ret + 1) * sizeof(char)));
    ret[len_ret] = '\0';
    sprintf(ret, "%s", url_base);

    for (int i = 0; i < idx; i++) {
        if (i == 0) {
            char *tmp_ret = strdup(ret);
            sprintf(ret, "%s%s", tmp_ret, list_args[i]);
            free(tmp_ret);
            continue;
        }
        char *tmp_ret = strdup(ret);
        sprintf(ret, "%s&%s", tmp_ret, list_args[i]);
        free(tmp_ret);
    }
    char *tmp_ret = strdup(ret);
    sprintf(ret, "%s%s", tmp_ret, mixin_key);
    free(tmp_ret);

    uint8_t w_rid_raw[16];
    char w_rid[33];
    w_rid[32] = '\0';
    md5String(ret, w_rid_raw);

    for (int i = 0; i < 16; i++) {
        sprintf(&w_rid[i * 2], "%02x", w_rid_raw[i]);
    }

    size_t len_ret_url = strlen(url_raw) + strlen(w_rid) + len_wts + 12;
    char *ret_url = (char *)malloc((len_ret_url + 1) * sizeof(char));

    ret_url[len_ret_url] = '\0';
    snprintf(ret_url, len_ret_url + 1, "%s&w_rid=%s&wts=%s", url_raw, w_rid,
             wts_str);

    free(ret);
    free(wts_str);
    return ret_url;
}