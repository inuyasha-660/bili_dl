#include "api/api.h"
#include "utils/utils.h"
#include <cJSON.h>
#include <stdlib.h>
#include <string.h>

extern struct Account *account;
struct Anime *anime;

int cfg_read_anime()
{
    int err = 0;
    anime = malloc(sizeof(struct Anime));
    anime->coding = NULL;
    anime->id = NULL;
    anime->mode = 0;
    anime->part = NULL;
    anime->qn = NULL;

    cJSON *root = cJSON_Parse(account->config_str);
    if (root == NULL) {
        err = ERR_PARSE;
        error("Failed to parse %s", account->config_path);
        goto end;
    }
    cJSON *Require = cJSON_GetObjectItemCaseSensitive(root, "Require");
    if (Require == NULL) {
        err = ERR_PARSE;
        error("Failed to parse(Require) %s", account->config_path);
        goto end;
    }

    cJSON *id = cJSON_GetObjectItemCaseSensitive(Require, "id");
    cJSON *part = cJSON_GetObjectItemCaseSensitive(Require, "part");
    cJSON *mode = cJSON_GetObjectItemCaseSensitive(Require, "mode");
    cJSON *qn = cJSON_GetObjectItemCaseSensitive(Require, "qn");
    cJSON *coding = cJSON_GetObjectItemCaseSensitive(Require, "coding");
    if (id == NULL || part == NULL || mode == NULL || qn == NULL ||
        coding == NULL) {
        error("(Require)Found a NULL value");
        err = ERR_PARSE;
        goto end;
    }
    if (!cJSON_IsString(id) || !cJSON_IsNumber(mode) || !cJSON_IsString(qn) ||
        !cJSON_IsString(coding)) {
        error("(Require) Found a value with invalid type");
        err = ERR_PARSE;
        goto end;
    }

    int size_p = cJSON_GetArraySize(part);
    anime->part = (int *)malloc((size_p + 1) * sizeof(int));
    for (int i = 0; i < size_p; i++) {
        cJSON *part_item = cJSON_GetArrayItem(part, i);
        if (part_item == NULL || !cJSON_IsNumber(part_item)) {
            error("(line(part): %d item: %d) Failed to get part", index + 1, i);
            err = ERR_PARSE;
            goto end;
        }
        anime->part[i] = part_item->valueint;
    }
    anime->part[size_p] = END_P;

    anime->id = strdup(id->valuestring);
    anime->mode = id->valueint;
    anime->qn = strdup(qn->valuestring);
    anime->coding = strdup(coding->valuestring);

end:
    cJSON_Delete(root);
    return err;
}