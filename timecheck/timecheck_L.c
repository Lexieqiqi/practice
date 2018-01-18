/*
 * ��ʱ�����
 * llm ,2018��1��17��16:06:24
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <time.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include "timecheck_L.h"

enum ACTION{
    TIMER_START,
    TIMER_STOP,
    TIMER_RESTART,
    TIMER_DEBUG,
    TIMER_SHOW,
    UNKNOW_TYPE
};

#define SERVER_SOCKET_PATH "/var/timecheck_l_usock"
#define FD_CNT_MAX 128

#define IN_RANGE(x, a, b) ((x) >= (a) && (x) <= (b))

int log_level = LLOG_INFO;

int all_fd[FD_CNT_MAX] = {0};

void add_fd(int fd)
{
    int i = 0;

    for(i = 0; i < FD_CNT_MAX; i++)
    {
        if(-1 == all_fd[i])
        {
            all_fd[i] = fd;
            return;
        }
    }

    if(i == FD_CNT_MAX)
    {
        llog(LLOG_ERR, "fd add faild,max fd num is %d\n", FD_CNT_MAX);
    }
}

void del_a_fd(int fd)
{
    int i = 0;

    for(i = 0; i < FD_CNT_MAX; i++)
    {
        if(fd == all_fd[i])
        {
            all_fd[i] = -1;
            return;
        }
    }
}

int get_max_fd(void)
{
    int max = -1;
    int i = 0;

    for(i = 0; i < FD_CNT_MAX; i++)
    {
        if(max < all_fd[i])
            max = all_fd[i];
    }

    return max;
}

/* �ж��������Ƿ���һ����ʱ��������Ƿ��ض�ʱ����ʱ��ָ�룬���򷵻�NULL */
timer_lt *fd_is_a_timer(fd)
{
    int i = 0;

    if(-1 == fd)
        return NULL;

    for(i = 0; i < timer_array_size; i++)
    {
        if(all_timer[i].fd == fd)
            return &all_timer[i];
    }

    return NULL;
}

void show_a_timer(timer_lt *t)
{
    printf("==========================================\n");
    printf("name: %s\n", t->name);
    printf("first time: %s\n", t->first_time);
    printf("interval: %ds\n", t->interval);
    printf("debug_flag: %d\n", t->debug_flag);
    printf("auto_start: %d\n", t->auto_start);
    printf("running_flag: %d\n", t->running);    
    printf("===========================================\n");
}

void show_all_timer(void)
{
    int i = 0;
    
    for(i = 0; i < timer_array_size; i++)
    {
        show_a_timer(&all_timer[i]);
    }
}

void sig_handle(int sig)
{
    if(sig == SIGCHLD)
    {
        while(waitpid(-1, NULL, WNOHANG) > 0);
    }
}

int init_server_sock(void)
{
    int fd;
    int server_len;
    int rtn;
    struct sockaddr_un	sockaddr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd < 0)
    {
        llog(LLOG_ERR, "create socket error, %s\n", strerror(errno));
        return -1;
    }

    unlink(SERVER_SOCKET_PATH);

    strncpy(sockaddr.sun_path, SERVER_SOCKET_PATH, sizeof(sockaddr.sun_path) - 1);
    sockaddr.sun_family = AF_UNIX ;
    server_len = sizeof(struct sockaddr_un);
    rtn = bind(fd, (struct sockaddr * )&sockaddr, server_len);
    if(rtn < 0)
    {
        llog(LLOG_ERR, "bind error, %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    rtn = listen(fd, 5);
    if(rtn < 0)
    {
        llog(LLOG_ERR, "listen error, %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

int connect_server(void)
{
    int fd;
    int server_len;
    int rtn;
    struct sockaddr_un	sockaddr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd < 0)
    {
        llog(LLOG_ERR, "create socket error, %s\n", strerror(errno));
        return -1;
    }

    strncpy(sockaddr.sun_path, SERVER_SOCKET_PATH, sizeof(sockaddr.sun_path) - 1);
    sockaddr.sun_family = AF_UNIX ;
    server_len = sizeof(struct sockaddr_un);

    rtn = connect(fd, (struct sockaddr * )&sockaddr, server_len);
    if(-1 == rtn)
    {
        close(fd);
        return -1;
    }
    else
    {
        return fd;
    }
}

int accept_cli(int server_fd)
{
    int	socklen;
    struct sockaddr_un cli_addr;
    socklen = sizeof( struct sockaddr_un);
    return accept(server_fd, (struct sockaddr *)&cli_addr, &socklen);
}

timer_lt *find_timer_by_name(char *name)
{
    int i = 0;

    if(!name)
        return NULL;
    
    for(i = 0; i < timer_array_size; i++)
    {
        if(!strcmp(name, all_timer[i].name))
            return &all_timer[i];
    }

    llog(LLOG_ERR, "not find the timer: %s\n", name);
    return NULL;
}


void run_client(char *timer_name, char *msg)
{
    timer_lt *t = NULL;
    int i = 0;
    int fd = 0;
    int rtn = 0;
    char buf[128] = {0};

    if(!timer_name || !msg)
        return;

    if(!find_timer_by_name(timer_name))
        return;

    fd = connect_server();
    if(fd < 0)
    {
        llog(LLOG_ERR, "connect fialed, %s\n", strerror(errno));
        return;
    }

    llog(LLOG_DEBUG, "connect server success\n");

    snprintf(buf, sizeof(buf), "%s %s", timer_name, msg);
    rtn = write(fd, buf, strlen(buf) + 1);
    if(rtn != strlen(buf) + 1)
    {
        llog(LLOG_ERR, "send err, %s\n", strerror(errno));
    }

    llog(LLOG_DEBUG, "send success\n");

    close(fd);
}

/* 0��ʾ���ڣ�-1������ */
int check_app_alive_by_pid(int pid)
{
    char buf[128] = {0};

    snprintf(buf, sizeof(buf), "/proc/%d", pid);
    return access(buf, O_RDONLY);    
}

/* ׼��timer handle�ӽ��̵����л��� */
void prepare_child_env(timer_lt *t)
{
    int fd = open("/", O_RDONLY);
    int i = 0;
    
    /* �رմ򿪵��ļ������� */
    for(i = 3; i <= fd; i++)
        close(fd);

    /* ����debug���� */
    if(t->debug_flag)
        log_level = LLOG_DEBUG;
    else
        log_level = LLOG_INFO;
}

void timer_handle(timer_lt *t)
{
    uint64_t exp_cnt = 0;

    /* 
        ���ص��������е�����������ִ���ʽ��
        1. ֱ���ظ����лص�
        2. �ȴ��ϴλص�������Ȼ���������лص�
        3. �ȴ��ϴλص���������һ��timeout�����лص�
    */
    
    /* �ж��Ƿ��timer�Ļص��Ƿ������� */
    if(!check_app_alive_by_pid(t->child_pid))
    {
        llog(LLOG_DEBUG, "The callback handle is alive! pid is %d\n", t->child_pid);

        switch(t->cb_pending_type)
        {
            case DUP_RUN_CB:
                break;
            case WAIT_RUN:
                /* ˯��10ms������������һֱ����select����״̬ռ��cpu */
                usleep(10*1000);
                return;
            case NEXT_RUN:
                /* ��ȡ��ʱ���� */
                read(t->fd, &exp_cnt, sizeof(uint64_t));
                /* ֱ�ӷ��أ��൱��û�����������ʱ */
                return;
        }
    }
    
    /* ��ȡ��ʱ����������select��һֱ���� */
    read(t->fd, &exp_cnt, sizeof(uint64_t));
    llog(LLOG_DEBUG, "timer[%s] expire count: %llu\n", t->name, (unsigned long long)exp_cnt);

    /* ׼��handle�����л��� */
    pid_t pid = fork();
    if(pid < 0)
    {
        llog(LLOG_ERR, "fork failed\n");
        return;
    }
    if(pid > 0)
    {
        t->child_pid = pid;
        return;
    }

    /* �ӽ��̿ռ� */
    prepare_child_env(t);
    if(t->cb)
        t->cb();
    exit(0);
}

/* ��ʼ��һ����ʱ����Դ�����ظö�ʱ�����ļ�������,ʧ�ܷ���-1 */
int start_a_timer(timer_lt *t)
{
    int first_run_sec = 0;
    int sec = 0;
    struct itimerspec new_value;
    int  fd;
    time_t now_sec;
    struct tm now;
    
    if(!t)
        return -1;

    /* �ж��Ƿ��Ѿ������������ʱ���� */
    if(t->running)
    {
        llog(LLOG_ERR, "can't start a timer twice\n");
        return -1;
    }

    /* �����Լ�����ĳ�ʼ������ */
    if(t->init)
        t->init(t);

    if(!strlen(t->first_time) || -1 == (sec = parse_time(t->first_time)))
    {
        first_run_sec = 0;  //û������ĵ�һ������ʱ��
    }
    else
    {
        now_sec = time(NULL);
        localtime_r(&now_sec, &now);  /* gmtime��UTCʱ�䣬localtime����ʱ�� */
        
        first_run_sec = (24*3600 + sec - (now.tm_hour*3600 + now.tm_min*60 + now.tm_sec))%(24*3600);
    }

    /* �����״γ�ʱʱ�� */
    /* �����Ϊ0��ʾֹͣ��ʱ�� */
    first_run_sec  = first_run_sec ? first_run_sec : t->interval;
    new_value.it_value.tv_sec = first_run_sec;   
    new_value.it_value.tv_nsec = 0;
	
	/* ���ó�ʱ��� */
    new_value.it_interval.tv_sec = t->interval;
    new_value.it_interval.tv_nsec = 0;

    llog(LLOG_INFO, "tiemr %s first run time: %dsec, interval: %dsec\n", t->name, 
            first_run_sec, t->interval);

    fd = timerfd_create(CLOCK_MONOTONIC, 0);
    t->fd = fd;
    
    if(fd == -1)
    {
        llog(LLOG_ERR, "timerfd_create error: %s\n", strerror(errno));
        return -1;
    }

    if(timerfd_settime(fd, 0, &new_value, NULL) == -1)
    {
        llog(LLOG_ERR, "timerfd_settime error: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    
    t->running = 1;
    t->child_pid = -1;
    if(!IN_RANGE(t->cb_pending_type, DUP_RUN_CB, NEXT_RUN))
        t->cb_pending_type = NEXT_RUN;  // Ĭ��ֵ
    add_fd(fd);
    return fd;
}

void stop_a_timer(timer_lt *t)
{
    /* �����timer��handle���������Ƿ���Ӱ�� */
    if(!t->running)
        return;
        
    t->running = 0;
    close(t->fd);
    del_a_fd(t->fd);
    t->fd = -1;
}

int recv_msg(int fd, char *buf, int len)
{
    int rtn = 0;

    do
    {
        rtn = read(fd, buf, len);
    }while(rtn == -1 && errno == EINTR);  /* ����Ϣ���ܱ��ӽ����˳����ź��ж� */
    
    return rtn;
}

/* �����յ�����Ϣ���ɹ��򷵻ظ���Ϣ��Ӧ�Ķ�ʱ���Ͷ��� */
int parse_recv_msg(char *msg, timer_lt **t, int *action)
{
    char name[128] = {0};
    char cmd[128] = {0};

    if(2 != sscanf(msg, "%127s%127s", name, cmd))
    {
        llog(LLOG_ERR, "invaild msg,%s\n", msg);
        return -1;
    }

    *t = find_timer_by_name(name);

    /* �������� */
    if(!strcmp(cmd, "start"))
        *action = TIMER_START;
    else if(!strcmp(cmd, "stop"))
        *action = TIMER_STOP;
    else if(!strcmp(cmd, "restart"))
        *action = TIMER_RESTART;
    else if(!strcmp(cmd, "debug"))
        *action = TIMER_DEBUG;
    else if(!strcmp(cmd, "show"))
        *action = TIMER_SHOW;
    else
        *action = UNKNOW_TYPE;

    if(!*t || *action == UNKNOW_TYPE)
        return -1;
    else
        return 0;
}

void server_sock_handle(int fd)
{
    char msg[128] = {0};
    int rtn = 0;
    timer_lt *t = NULL;
    int action = 0;

    rtn = recv_msg(fd, msg, sizeof(msg) - 1);
    close(fd);
    del_a_fd(fd);
    llog(LLOG_INFO, "recv %d bytes msg: %s\n", rtn, msg);

    if(-1 == parse_recv_msg(msg, &t, &action))
    {
        llog(LLOG_ERR, "msg parse failed,%s\n", msg);
        return;
    }

    /* �Լ���debug��Ϣ */
    if(!strcmp(t->name, "own"))
    {
        if(action == TIMER_DEBUG)
            log_level = log_level == LLOG_DEBUG ? LLOG_INFO : LLOG_DEBUG;
        else if(action == TIMER_SHOW)
            show_all_timer();
        return;
    }

    switch(action)
    {
        case TIMER_START:
            start_a_timer(t);
            llog(LLOG_INFO, "start timer(%s)\n", t->name);
            break;
        case TIMER_STOP:
            stop_a_timer(t);
            llog(LLOG_INFO, "stop timer(%s)\n", t->name);
            break;
        case TIMER_RESTART:
            stop_a_timer(t);
            start_a_timer(t);
            llog(LLOG_INFO, "restart timer(%s)\n", t->name);
            break;
        case TIMER_DEBUG:
            t->debug_flag = !t->debug_flag;
            llog(LLOG_INFO, "set timer[%s] debug %s\n", t->name, t->debug_flag ? "on" : "off");
            break;
        case TIMER_SHOW:
            show_a_timer(t);
            break;
        default:
            ;
    }
    
}

/* ����hh:mm:ssʱ���Ϊ0�㵽��ʱ����������ʧ�ܷ���-1 */
int parse_time(char *time)
{
    int h = 0, m = 0, s = 0;
    
    if(!time)
        return -1;
        
    if(sscanf(time, "%d:%d:%d", &h, &m, &s) != 3)
    {
        llog(LLOG_ERR, "parse time error: %s\n", time);
        return -1;
    }

    if(!IN_RANGE(h, 0, 23) || !IN_RANGE(m, 0, 59) || !IN_RANGE(s, 0, 59))
    {
        llog(LLOG_ERR, "parse time error: %s\n", time);
        return -1;
    }
    
    llog(LLOG_DEBUG, "timer start time: %dh%dm%ds\n", h, m, s);
    return (h*3600 + m*60 + s);
}

void init_all_fd(void)
{
    int i = 0;
    
    for(i = 0; i < FD_CNT_MAX; i++)
    {
        all_fd[i] = -1;
    }

}

void init_all_timer(void)
{
    int i = 0;
    int fd = 0;

    for(i = 0; i < timer_array_size; i++)
    {
        if(all_timer[i].auto_start)
        {
            fd = start_a_timer(&all_timer[i]);
            llog(LLOG_INFO, "init timer %s %s\n", all_timer[i].name, fd == -1 ? "faild" : "success");
        }
    }
}

void run_server(void)
{
    int server_fd = 0;
    struct timeval tv = {5, 0};
    fd_set fd_read;
    int i = 0;
    timer_lt *t;

    signal(SIGCHLD, sig_handle);

    init_all_fd();
    
    /* ��ʼ����ʱ�� */
    init_all_timer();
    
    /* ��ʼ��sock */
    server_fd = init_server_sock();
    if(-1 == server_fd)
    {
        llog(LLOG_ERR, "init server sock failed\n");
        return;
    }
    add_fd(server_fd);

    while(1)
    {
        FD_ZERO(&fd_read) ;
        
        for(i = 0; i < FD_CNT_MAX; i++)
        {
            if(-1 != all_fd[i])
                FD_SET(all_fd[i], &fd_read);
        }
        
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        if(0 >= select(get_max_fd() + 1, &fd_read, NULL, NULL, &tv))
            continue;

        llog(LLOG_DEBUG, "select weakup\n");

        for(i = 0; i < FD_CNT_MAX; i++)
        {
            if(-1 != all_fd[i] && FD_ISSET(all_fd[i], &fd_read))
            {
                /* �ж����fd��timer�Ļ���server��sock */
                if((t = fd_is_a_timer(all_fd[i])))
                {
                    timer_handle(t);
                }
                else
                {
                    if(server_fd == all_fd[i])
                        add_fd(accept_cli(server_fd));
                    else    
                        server_sock_handle(all_fd[i]);
                }
            }
        }
    }
}

void show_help(void)
{
    printf("timecheck_L [timer name] [action]\n");
    printf("action support: start, stop, restart, show, debug\n\n");
    printf("eg: timecheck_L own show\n");
    printf("eg: timecheck_L internet_check start\n");
    printf("eg: timecheck_L internet_check stop\n");
    printf("eg: timecheck_L internet_check restart\n");
    printf("eg: timecheck_L internet_check debug\n");
}

int main(int argc, char *argv[])
{
    if(argc == 1)
    {
        run_server();
    }
    else if(argc == 3)
    {
        run_client(argv[1], argv[2]);
    }
    else if(argc == 2 && !strcmp(argv[1], "show"))
    {
        show_all_timer();
    }
    else
    {
        show_help();
    }
    
    return 0;
}



