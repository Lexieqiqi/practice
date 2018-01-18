#ifndef _TIMECHECK_L_H_
#define _TIMECHECK_L_H_

enum LEVEL
{
    LLOG_ERR,
    LLOG_INFO,
    LLOG_DEBUG
};

enum CB_PENDING_TYPE{
    DUP_RUN_CB = 100,   /* ֱ���ظ����� */
    WAIT_RUN,           /* �ȴ��ϴλص���������������λص� */
    NEXT_RUN            /* �ȴ��ϴλص������������ص������ǵȵ��´γ�ʱ��Ĭ�������ַ�ʽ�� */
};

#define PIPE_BUF_SIZE 1024

typedef struct timer_l{
    char name[128];         //��ʱ������
    char first_time[32];    //��һ��run��ʱ�䣬��ʽ"hh:mm:ss"
    int interval;  //�������еļ����0��ʾ���������
    int debug_flag;         //debug��־
    void (*init)(struct timer_l *t);;   //��ʼ������
    void (*cb)(struct timer_l *t);       //�ص�
    int auto_start;         //���������������ͨ����Ϣ����
    int fd;                 //��timer��Ӧ���ļ�������
    int running;            //��ʱ���������ı�־
    int child_pid;          //�ӽ���pid
    int cb_pending_type;    //���ص��������е�����£���һ�γ�ʱ�����˵Ĵ�������
    char priv_data[PIPE_BUF_SIZE];   //�ӽ��̱����˽������
    //int priv_data_len;      //˽�����ݳ���
    int pipe_read_fd;       //���ӽ��̽����Ķ��ܵ�
}timer_lt; 

extern int log_level;

#define llog(level, format, argv...) do{    \
        if(level <= log_level)  \
            printf("[%s] %s(%d) "format, #level, __FUNCTION__, __LINE__, ##argv);   \
    }while(0)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

extern timer_lt all_timer[];
extern int timer_array_size;

#endif /* _TIMECHECK_L_H_ */