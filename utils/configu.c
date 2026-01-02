#include "utils.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct Account *account;

int cfg_read_global()
{
    if (!is_file_exists(account->config_path)) {
        error("%s not found", account->config_path);
        return FE_OP;
    }
    char *config = read_file(account->config_path);
    if (config == NULL) {
        error("%s is NULL", account->config_path);
        return TYPEE;
    }
    account->config_str = strdup(config);

    int err = 0;
    cJSON *root = cJSON_Parse(config);
    if (root == NULL) {
        error("Fail to parse %s", account->config_path);
        err = PARSE;
        goto end;
    }
    cJSON *SESSDATA = cJSON_GetObjectItemCaseSensitive(root, "SESSDATA");
    if (SESSDATA == NULL || SESSDATA->valuestring == NULL) {
        error("SESSDATA is NULL");
        err = PARSE;
        goto end;
    }
    cJSON *MaxThread = cJSON_GetObjectItemCaseSensitive(root, "MaxThread");
    if (MaxThread == NULL || !cJSON_IsNumber(MaxThread)) {
        error("MaxThread is NaN");
        err = PARSE;
        goto end;
    }
    cJSON *Type = cJSON_GetObjectItemCaseSensitive(root, "Type");
    if (Type == NULL || !cJSON_IsNumber(Type)) {
        error("Type is NULL");
        err = PARSE;
        goto end;
    }
    cJSON *Output = cJSON_GetObjectItemCaseSensitive(root, "Output");
    if (Output == NULL || Output->valuestring == NULL) {
        error("Output is NULL");
        err = PARSE;
        goto end;
    }
    cJSON *Requires = cJSON_GetObjectItemCaseSensitive(root, "Require");
    if (Requires == NULL) {
        error("Require is NULL");
        err = PARSE;
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