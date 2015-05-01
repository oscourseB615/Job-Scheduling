#ifndef JOB_H
#define JOB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>

#ifndef DEBUG
#define DEBUG
#endif

#undef DEBUG

#define BUFLEN 100
#define GLOBALFILE "screendump"

enum jobstate{
    READY,RUNNING,DONE
};

enum cmdtype{
    ENQ=-1,DEQ=-2,STAT=-3 
};
struct jobcmd{
    enum cmdtype type;
    int argnum;
    int owner;
    int defpri;
    char data[BUFLEN];
};

#define DATALEN sizeof(struct jobcmd)

struct jobinfo{
    int jid;              /* 作业ID */
    int pid;              /* 进程ID */
    char** cmdarg;        /* 命令参数 */
    int defpri;           /* 默认优先级 */
    int curpri;           /* 当前优先级 */
    int ownerid;          /* 作业所有者ID */
    int wait_time;        /* 作业在等待队列中等待时间 */
    time_t create_time;   /* 作业创建时间 */
    int run_time;         /* 作业运行时间 */
    enum jobstate state;  /* 作业状态 */
};

struct waitqueue{
    struct waitqueue *next;
    struct jobinfo *job;
};

void scheduler();
void sig_handler(int sig,siginfo_t *info,void *notused);
int allocjid();
void add_queue(struct jobinfo *job);
void del_queue(struct jobinfo *job);
void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd);
void do_deq(struct jobcmd deqcmd);
void do_stat(struct jobcmd statcmd);
void updateall();
struct waitqueue* jobselect();
void jobswitch();

void error_doit(int errnoflag,const char *fmt,va_list ap);
void error_sys(const char *fmt,...);
void error_msg(const char *fmt,...);
void error_quit(const char *fmt,...);

#endif

