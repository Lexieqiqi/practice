#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "timecheck_L.h"

void test_timer_cb(void)
{
    llog(LLOG_DEBUG, "test_timer_cb cb\n");
    system("date");
}

void test_timer_init(timer_lt *t)
{
    time_t now_sec;
    struct tm now;
    char buf[128] = {0};

    /* ���õ�һ���ӳ�30s�ſ�ʼ���� */
    now_sec = time(NULL) + 30;
    localtime_r(&now_sec, &now);
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.tm_hour, now.tm_min, now.tm_sec);

    /* �������ö�ʱ������ */
    strcpy(t->first_time, buf);
    t->interval = 2;
    
    llog(LLOG_INFO, "set first run time: %s, interval: %ds\n", buf, t->interval);
}