#include "api/api.h"
#include "utils/utils.h"
#include <cJSON.h>
#include <stdlib.h>
#include <string.h>

extern struct Video *video_s;
struct Part *partl;
extern struct Account *account;

// 读取 Video 数组， VideoObjIn 应由传入者释放
int cfg_read_video(cJSON *VideoObjIn)
{
    int err = 0;

    cJSON *root = NULL;
    cJSON *VideoObj = NULL;
    if (VideoObjIn == NULL) {
        root = cJSON_Parse(account->config_str);
        if (root == NULL) {
            error("Failed to parse %s", account->config_path);
            goto end;
        }
        VideoObj = cJSON_GetObjectItemCaseSensitive(root, "Require");
        if (VideoObj == NULL) {
            error("Failed to parse Require");
            err = 3;
            goto end;
        }
    } else {
        VideoObj = VideoObjIn;
    }

    video_s = malloc(sizeof(struct Video));
    video_s->count = 0;
    video_s->Bvid = NULL;
    video_s->part = NULL;
    video_s->mode = NULL;
    video_s->audio = NULL;
    video_s->qn = NULL;
    video_s->coding = NULL;

    cJSON *require;
    int index = 0;
    cJSON_ArrayForEach(require, VideoObj)
    {
        cJSON *Bvid = cJSON_GetObjectItemCaseSensitive(require, "Bvid");
        cJSON *part = cJSON_GetObjectItemCaseSensitive(require, "part");
        cJSON *mode = cJSON_GetObjectItemCaseSensitive(require, "mode");
        cJSON *qn = cJSON_GetObjectItemCaseSensitive(require, "qn");
        cJSON *audio = cJSON_GetObjectItemCaseSensitive(require, "audio");
        cJSON *coding = cJSON_GetObjectItemCaseSensitive(require, "coding");
        if (Bvid == NULL || part == NULL || mode == NULL || qn == NULL ||
            audio == NULL || coding == NULL) {
            error("(line(req): %d) Found a NULL value", index + 1);
            err = 3;
            goto end;
        }
        if (!cJSON_IsString(Bvid) || !cJSON_IsNumber(mode) ||
            !cJSON_IsString(qn) || !cJSON_IsString(audio) ||
            !cJSON_IsString(coding)) {
            error("(line(req): %d) Found a value with invalid type\n",
                  index + 1);
            err = 3;
            goto end;
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
        video_s->part[index] = NULL;

        int size = cJSON_GetArraySize(part);
        video_s->part[index] = (int *)malloc((size + 1) * sizeof(int));
        for (int i = 0; i < size; i++) {
            cJSON *part_item = cJSON_GetArrayItem(part, i);
            if (part_item == NULL || !cJSON_IsNumber(part_item)) {
                error("(line(part): %d item: %d) Failed to get part\n",
                      index + 1, i);
                err = 3;
                goto end;
            }

            video_s->part[index][i] = part_item->valueint;
        }
        video_s->part[index][size] = END_P; // 作为哨兵值

        video_s->Bvid[index] = strdup(Bvid->valuestring);
        video_s->mode[index] = mode->valueint;
        video_s->qn[index] = strdup(qn->valuestring);
        video_s->audio[index] = strdup(audio->valuestring);
        video_s->coding[index] = strdup(coding->valuestring);

        index += 1;
    }
    video_s->count = index;

end:
    if (root != NULL) {
        cJSON_Delete(root);
    }
    return err;
}
