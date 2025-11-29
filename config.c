#include "config.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct Account *account;

int is_file_exists(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        fclose(file);
        return 1;
    } else {
        return 0;
    }
}

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Fail to open %s\n", filename);
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    char *ret = (char *)malloc((size + 1) * sizeof(char));
    fseek(file, 0L, SEEK_SET);

    int inx = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        ret[inx] = ch;
        inx++;
    }
    ret[inx] = '\0';
    fclose(file);

    return ret;
}

// 读取 Type = 1 时的 Require 数组
static int cfg_read_video(char *config)
{
    int err = 0;
    cJSON *root = cJSON_Parse(config);
    if (root == NULL) {
        fprintf(stderr, " Error: Failed to parse %s\n", account->config_path);
        err = 3;
        goto end;
    }

    account->video = malloc(sizeof(struct Video));
    account->video->count = 0;
    account->video->Bvid = NULL;
    account->video->part = NULL;
    account->video->mode = NULL;
    account->video->part = (int **)malloc(sizeof(int *));
    account->video->mode = (int *)malloc(sizeof(int));
    account->video->audio = NULL;
    account->video->qn = NULL;
    account->video->coding = NULL;

    cJSON *Requires = cJSON_GetObjectItemCaseSensitive(root, "Require");
    cJSON *require;
    int index = 0;
    cJSON_ArrayForEach(require, Requires)
    {
        cJSON *Bvid = cJSON_GetObjectItemCaseSensitive(require, "Bvid");
        cJSON *part = cJSON_GetObjectItemCaseSensitive(require, "part");
        cJSON *mode = cJSON_GetObjectItemCaseSensitive(require, "mode");
        cJSON *qn = cJSON_GetObjectItemCaseSensitive(require, "qn");
        cJSON *audio = cJSON_GetObjectItemCaseSensitive(require, "audio");
        cJSON *coding = cJSON_GetObjectItemCaseSensitive(require, "coding");
        if (Bvid == NULL || part == NULL || mode == NULL || qn == NULL || audio == NULL || coding == NULL) {
            fprintf(stderr, "Error: (line: %d) Found a NULL value\n", index + 1);
            err = 3;
            goto end;
        }
        if (!cJSON_IsString(Bvid) || !cJSON_IsNumber(mode) || !cJSON_IsString(qn) || !cJSON_IsString(audio) ||
            !cJSON_IsString(coding)) {
            fprintf(stderr, "Error: (line: %d) Found a value with invalid type\n", index + 1);
            err = 3;
            goto end;
        }

        account->video->Bvid = (char **)realloc(account->video->Bvid, (index + 1) * sizeof(char *));
        account->video->mode = (int *)realloc(account->video->mode, (index + 1) * sizeof(int));
        account->video->part = (int **)realloc(account->video->part, (index + 1) * sizeof(int *));
        account->video->audio = (char **)realloc(account->video->audio, (index + 1) * sizeof(char *));
        account->video->qn = (char **)realloc(account->video->qn, (index + 1) * sizeof(char *));
        account->video->coding = (char **)realloc(account->video->coding, (index + 1) * sizeof(char *));
        account->video->part[index] = NULL;

        int size = cJSON_GetArraySize(part);
        for (int i = 0; i < size; i++) {
            cJSON *part_item = cJSON_GetArrayItem(part, i);
            if (part_item == NULL || !cJSON_IsNumber(part_item)) {
                fprintf(stderr, "Error: (line: %d item: %d) Failed to get part\n", index + 1, i);
                err = 3;
                goto end;
            }
            account->video->part[index] = (int *)realloc(account->video->part[index], (i + 1) * sizeof(int));
            account->video->part[index][i] = part_item->valueint;
        }

        account->video->Bvid[index] = strdup(Bvid->valuestring);
        account->video->mode[index] = mode->valueint;
        account->video->qn[index] = strdup(qn->valuestring);
        account->video->audio[index] = strdup(audio->valuestring);
        account->video->coding[index] = strdup(coding->valuestring);

        index += 1;
    }
    account->video->count = index;

end:
    cJSON_Delete(root);

    return err;
}

int cfg_read()
{
    if (!is_file_exists(account->config_path)) {
        fprintf(stderr, "Error: %s not found\n", account->config_path);
        return -1;
    }
    char *config = read_file(account->config_path);
    if (config == NULL) {
        fprintf(stderr, "Error: %s is NULL\n", account->config_path);
        return -1;
    }

    int err = 0;
    cJSON *root = cJSON_Parse(config);
    if (root == NULL) {
        fprintf(stderr, "Error: Fail to parse %s\n", account->config_path);
        err = 1;
        goto end;
    }
    cJSON *SESSDATA = cJSON_GetObjectItemCaseSensitive(root, "SESSDATA");
    if (SESSDATA == NULL || SESSDATA->valuestring == NULL) {
        fprintf(stderr, "Error: SESSDATA is NULL\n");
        err = 1;
        goto end;
    }
    cJSON *MaxThread = cJSON_GetObjectItemCaseSensitive(root, "MaxThread");
    if (MaxThread == NULL || !cJSON_IsNumber(MaxThread)) {
        fprintf(stderr, " Error: MaxThread is NaN\n");
        err = 1;
        goto end;
    }
    cJSON *Type = cJSON_GetObjectItemCaseSensitive(root, "Type");
    if (Type == NULL || !cJSON_IsNumber(Type)) {
        fprintf(stderr, "Error: Type is NULL\n");
        err = 1;
        goto end;
    }
    cJSON *Output = cJSON_GetObjectItemCaseSensitive(root, "Output");
    if (Output == NULL || Output->valuestring == NULL) {
        fprintf(stderr, "Error: Output is NULL\n");
        err = 1;
        goto end;
    }

    account->SESSDATA = strdup(SESSDATA->valuestring);
    account->MaxThread = MaxThread->valueint;
    account->Type = Type->valueint;
    account->Outoput = strdup(Output->valuestring);
    // 生成 cookie
    size_t len = snprintf(NULL, 0, "SESSDATA=%s", SESSDATA->valuestring);
    char *cookie = (char *)malloc((len + 1) * sizeof(char));
    snprintf(cookie, len + 1, "SESSDATA=%s", SESSDATA->valuestring);
    account->cookie = strdup(cookie);
    free(cookie);

    // 根据请求类型获取 Require
    switch (account->Type) {
    case 1: {
        err = cfg_read_video(config);
        break;
    }
    default: {
        err = 2;
        fprintf(stderr, "Error: Not matched Type\n");
        goto end;
    }
    }

end:
    free(config);
    cJSON_Delete(root);

    return err;
}