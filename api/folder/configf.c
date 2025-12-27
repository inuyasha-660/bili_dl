#include "api/api.h"
#include "utils/utils.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Folder *folder;
extern struct Account *account;

int cfg_read_folder()
{
    int err = 0;
    cJSON *root = cJSON_Parse(account->config_str);
    if (root == NULL) {
        fprintf(stderr, " Error: Failed to parse %s\n", account->config_path);
        err = 3;
        goto end;
    }

    cJSON *Require = cJSON_GetObjectItemCaseSensitive(root, "Require");
    if (Require == NULL) {
        fprintf(stderr, "Error: Failed to parse Require\n");
        err = 3;
        goto end;
    }

    folder = malloc(sizeof(struct Folder));
    folder->fid = NULL;
    folder->qn = NULL;
    folder->mode = 0;
    folder->coding = NULL;
    folder->audio = NULL;

    cJSON *fid = cJSON_GetObjectItemCaseSensitive(Require, "fid");
    cJSON *mode = cJSON_GetObjectItemCaseSensitive(Require, "mode");
    cJSON *qn = cJSON_GetObjectItemCaseSensitive(Require, "qn");
    cJSON *audio = cJSON_GetObjectItemCaseSensitive(Require, "audio");
    cJSON *coding = cJSON_GetObjectItemCaseSensitive(Require, "coding");
    if (fid == NULL || mode == NULL || qn == NULL || audio == NULL ||
        coding == NULL) {
        fprintf(stderr, "Error: Found a NULL value\n");
        err = 3;
        goto end;
    }
    if (!cJSON_IsString(fid) || !cJSON_IsNumber(mode) || !cJSON_IsString(qn) ||
        !cJSON_IsString(audio) || !cJSON_IsString(coding)) {
        fprintf(stderr, "Error: Found a value with invalid type\n");
        err = 3;
        goto end;
    }

    cJSON *excepts = cJSON_GetObjectItemCaseSensitive(Require, "except");
    cJSON *except;
    int idx_e = 0;
    cJSON_ArrayForEach(except, excepts)
    {
        cJSON *Bvid = cJSON_GetObjectItemCaseSensitive(except, "Bvid");
        cJSON *part = cJSON_GetObjectItemCaseSensitive(except, "part");
        cJSON *mode = cJSON_GetObjectItemCaseSensitive(except, "mode");
        cJSON *qn = cJSON_GetObjectItemCaseSensitive(except, "qn");
        cJSON *audio = cJSON_GetObjectItemCaseSensitive(except, "audio");
        cJSON *coding = cJSON_GetObjectItemCaseSensitive(except, "coding");
        if (Bvid == NULL || part == NULL || mode == NULL || qn == NULL ||
            audio == NULL || coding == NULL) {
            fprintf(stderr, "Error: (line: %d) Found a NULL value\n",
                    idx_e + 1);
            err = 3;
            goto end;
        }
        if (!cJSON_IsString(Bvid) || !cJSON_IsNumber(mode) ||
            !cJSON_IsString(qn) || !cJSON_IsString(audio) ||
            !cJSON_IsString(coding)) {
            fprintf(stderr,
                    "Error: (line: %d) Found a value with invalid type\n",
                    idx_e + 1);
            err = 3;
            goto end;
        }
        idx_e++;
    }

    folder->fid = strdup(fid->valuestring);
    folder->qn = strdup(qn->valuestring);
    folder->mode = mode->valueint;
    folder->coding = strdup(coding->valuestring);
    folder->audio = strdup(audio->valuestring);

end:
    cJSON_Delete(root);

    return err;
}