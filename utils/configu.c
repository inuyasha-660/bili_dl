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
        return ERR_FOP;
    }

    // 加载配置文件
    char *config_str = read_file(account->config_path);
    if (config_str == NULL) {
        error("%s is NULL", account->config_path);
        return ERR_TYPE;
    }
    account->config_str = strdup(config_str);

    int    err = 0;
    cJSON *root = cJSON_Parse(config_str);
    if (root == NULL) {
        error("Failed to parse root");
        err = ERR_PARSE;
        goto end;
    }
    // 获取 Cookie
    cJSON *SESSDATA = cJSON_GetObjectItemCaseSensitive(root, "SESSDATA");
    if (SESSDATA == NULL || SESSDATA->valuestring == NULL) {
        error("SESSDATA is NULL");
        err = ERR_PARSE;
        goto end;
    }
    // 获取最大线程数
    cJSON *MaxThread = cJSON_GetObjectItemCaseSensitive(root, "MaxThread");
    if (MaxThread == NULL || !cJSON_IsNumber(MaxThread)) {
        error("MaxThread is not a valid number");
        err = ERR_PARSE;
        goto end;
    }
    // 获取下载类型
    cJSON *Type = cJSON_GetObjectItemCaseSensitive(root, "Type");
    if (Type == NULL || !cJSON_IsNumber(Type)) {
        error("Type is NULL");
        err = ERR_PARSE;
        goto end;
    }
    // 获取输出目录
    cJSON *Output = cJSON_GetObjectItemCaseSensitive(root, "Output");
    if (Output == NULL || Output->valuestring == NULL) {
        error("Output is NULL");
        err = ERR_PARSE;
        goto end;
    }

    // 生成 Cookie
    size_t len_cookie = snprintf(NULL, 0, "SESSDATA=%s", SESSDATA->valuestring);
    char  *cookie = (char *)malloc((len_cookie + 1) * sizeof(char));
    snprintf(cookie, len_cookie + 1, "SESSDATA=%s", SESSDATA->valuestring);

    account->SESSDATA = strdup(SESSDATA->valuestring);
    account->MaxThread = MaxThread->valueint;
    account->Type = Type->valueint;
    account->Output = strdup(Output->valuestring);
    account->cookie = strdup(cookie);

    free(cookie);

end:
    free(config_str);
    cJSON_Delete(root);

    return err;
}