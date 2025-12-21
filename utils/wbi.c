#include "api/api.h"
#include "libs/md5.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct Wbi *wbi;

const int MIXIN_KEY_ENC_TAB[64] = {
    46, 47, 18, 2,  53, 8,  23, 32, 15, 50, 10, 31, 58, 3,  45, 35,
    27, 43, 5,  49, 33, 9,  42, 19, 29, 28, 14, 39, 12, 38, 41, 13,
    37, 48, 7,  16, 24, 55, 40, 61, 26, 17, 0,  1,  60, 51, 30, 4,
    22, 25, 54, 21, 56, 59, 6,  63, 57, 62, 11, 36, 20, 34, 44, 52};

int api_get_wbi_key()
{
    int err = 0;
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
            sprintf(ret, "%s%s", ret, list_args[i]);
            continue;
        }
        sprintf(ret, "%s&%s", ret, list_args[i]);
    }
    sprintf(ret, "%s%s", ret, mixin_key);

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