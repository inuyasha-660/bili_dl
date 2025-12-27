#include "utils.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct Account *account;

int cfg_read_global()
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
    account->config_str = strdup(config);

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
    account->Output = strdup(Output->valuestring);
    // 生成 cookie
    size_t len = snprintf(NULL, 0, "SESSDATA=%s", SESSDATA->valuestring);
    char *cookie = (char *)malloc((len + 1) * sizeof(char));
    snprintf(cookie, len + 1, "SESSDATA=%s", SESSDATA->valuestring);
    account->cookie = strdup(cookie);
    free(cookie);

end:
    free(config);
    cJSON_Delete(root);

    return err;
}