#include "utils/utils.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>

const char *LogLevelStr[3] = {"Info", "Warn", "Error"};

const char *LogLevelColor[4] = {
    "\x1b[1;32m", // 绿色
    "\x1b[1;33m", // 黄色
    "\x1b[1;31m", // 红色
    "\x1b[1;35m"  // 紫色(func)
};

void _log(int log_level, const char *func, char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    FILE *out = stdout;
    if (log_level == 2)
        out = stderr;

    pthread_mutex_lock(&lock);

    fprintf(out, "[%s%s\x1b[0m](%s%s\x1b[0m) ", LogLevelColor[log_level],
            LogLevelStr[log_level], LogLevelColor[3], func);
    vfprintf(out, format, ap);
    fprintf(out, "\n");
    va_end(ap);

    pthread_mutex_unlock(&lock);
}
