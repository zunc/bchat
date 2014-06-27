// ref: http://stackoverflow.com/questions/7411301/how-to-introduce-date-and-time-in-log-file
#include <time.h>

char* get_formatted_time(void) {

    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Must be static, otherwise won't work
    static char _retval[20];
    strftime(_retval, sizeof (_retval), "%Y-%m-%d %H:%M:%S", timeinfo);

    return _retval;
}