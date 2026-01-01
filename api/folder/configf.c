#include "api/api.h"
#include "utils/utils.h"
#include <cJSON.h>
#include <stdlib.h>
#include <string.h>

struct Folder *folder;
extern struct Video *video_s;
extern struct Account *account;

int cfg_read_folder()
{
    int err = 0;
    cJSON *root = cJSON_Parse(account->config_str);
    if (root == NULL) {
        error(" Failed to parse %s", account->config_path);
        err = 3;
        goto end;
    }

    cJSON *Require = cJSON_GetObjectItemCaseSensitive(root, "Require");
    if (Require == NULL) {
        error("Failed to parse Require");
        err = 3;
        goto end;
    }

    folder = malloc(sizeof(struct Folder));
    folder->fid = NULL;
    folder->qn = NULL;
    folder->mode = 0;
    folder->coding = NULL;
    folder->audio = NULL;

    video_s = malloc(sizeof(struct Video));
    video_s->count = 0;
    video_s->Bvid = NULL;
    video_s->part = NULL;
    video_s->mode = NULL;
    video_s->audio = NULL;
    video_s->qn = NULL;
    video_s->coding = NULL;

    cJSON *fid = cJSON_GetObjectItemCaseSensitive(Require, "fid");
    cJSON *mode = cJSON_GetObjectItemCaseSensitive(Require, "mode");
    cJSON *qn = cJSON_GetObjectItemCaseSensitive(Require, "qn");
    cJSON *audio = cJSON_GetObjectItemCaseSensitive(Require, "audio");
    cJSON *coding = cJSON_GetObjectItemCaseSensitive(Require, "coding");
    if (fid == NULL || mode == NULL || qn == NULL || audio == NULL ||
        coding == NULL) {
        error("Found a NULL value");
        err = 3;
        goto end;
    }
    if (!cJSON_IsString(fid) || !cJSON_IsNumber(mode) || !cJSON_IsString(qn) ||
        !cJSON_IsString(audio) || !cJSON_IsString(coding)) {
        error("Found a value with invalid type");
        err = 3;
        goto end;
    }

    // 读取 except 并填充 video_s
    cJSON *excepts = cJSON_GetObjectItemCaseSensitive(Require, "except");
    cfg_read_video(excepts);

    folder->fid = strdup(fid->valuestring);
    folder->qn = strdup(qn->valuestring);
    folder->mode = mode->valueint;
    folder->coding = strdup(coding->valuestring);
    folder->audio = strdup(audio->valuestring);

    Buffer *buffer_f = api_get_folder_ctn_json();
    if (buffer_f->buffer == NULL) {
        err = 3;
        error("Failed to get folder json");
        goto end;
    }

    cJSON *root_f = cJSON_Parse(buffer_f->buffer);
    if (root_f == NULL) {
        error(" Failed to parse root_f");
        err = 3;
        goto free_buffer;
    }
    cJSON *data = cJSON_GetObjectItemCaseSensitive(root_f, "data");
    if (data == NULL) {
        error("Failed to parse data");
        err = 3;
        goto free_buffer;
    }

    cJSON *videoobj;
    int index = video_s->count;
    int num_except = video_s->count;
    cJSON_ArrayForEach(videoobj, data)
    {
        cJSON *bvid = cJSON_GetObjectItemCaseSensitive(videoobj, "bvid");
        if (bvid == NULL || !cJSON_IsString(bvid)) {
            error("Failed to parse videoobj");
            err = 3;
            goto free_buffer;
        }

        for (int i = 0; i < num_except; i++) {
            if (!strcmp(bvid->valuestring, video_s->Bvid[i]))
                goto next;
        }

        video_s->Bvid =
            (char **)realloc(video_s->Bvid, (index + 1) * sizeof(char *));
        video_s->mode =
            (int *)realloc(video_s->mode, (index + 1) * sizeof(int));
        video_s->part =
            (int **)realloc(video_s->part, (index + 1) * sizeof(int *));
        video_s->audio =
            (char **)realloc(video_s->audio, (index + 1) * sizeof(char *));
        video_s->qn =
            (char **)realloc(video_s->qn, (index + 1) * sizeof(char *));
        video_s->coding =
            (char **)realloc(video_s->coding, (index + 1) * sizeof(char *));

        video_s->Bvid[index] = strdup(bvid->valuestring);
        video_s->qn[index] = folder->qn;
        video_s->mode[index] = folder->mode;
        video_s->audio[index] = folder->audio;
        video_s->coding[index] = folder->coding;
        video_s->part[index] = NULL;
        video_s->part[index] =
            (int *)realloc(video_s->part[index], 2 * sizeof(int));
        video_s->part[index][0] = 0;
        video_s->part[index][1] = END_P;

        index++;
    next: {
    }
    }
    video_s->count = index;

free_buffer:
    free(buffer_f->buffer);
    free(buffer_f);
    cJSON_Delete(root_f);

end:
    cJSON_Delete(root);
    return err;
}
