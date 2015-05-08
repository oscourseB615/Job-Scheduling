﻿#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "job.h"

#define DEBUG
#undef DEBUG
#define IMPSEGMT
int jobid=0;
int siginfo=1;
int fifo;
int globalfd;
int flag = 1;  			//标识第一个程序
int timeslice = -1;		//当前时间片的大小
int timecount = -1;		//当前已经运行的时间长度
int head_id = -1, oldhead_id = -1;
						//	各个队列的信息
						/* 			head_1	head_2 	head_3	head
							prio	H 		M 		L		current
							time 	1 		2 		5 		current
						*/
struct waitqueue *head_1=NULL, *head_2=NULL, *head_3=NULL, *head=NULL, *oldhead = NULL;		
struct waitqueue *next=NULL, *current =NULL;

/* 调度程序 */
void scheduler()
{
	struct jobinfo *newjob=NULL;
	struct jobcmd cmd;
	char timebuf[BUFLEN];
	int  count = 0, enqflag = 0;
	bzero(&cmd,DATALEN);
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");

	/*读取其他进程发来的信息*/
	#ifdef DEBUG
		printf("Reading whether other process send command!\n");
		if(count){
			printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
		}
		else
			printf("no data read\n");
	#endif

	/* 更新等待队列中的作业信息 */
	#ifdef DEBUG

		printf("Update jobs in wait queue!\n");
	#endif
	updateall();	//更新当前正在执行的 等待 队列的信息
	switch(cmd.type){
	case ENQ:
		/*执行ENQ*/
		#ifdef DEBUG
			printf("Execute enq!\n");
		#endif
		#ifdef DEBUG
			printf("Job Status Before ENQ\n");
			do_stat(cmd);
		#endif
		do_enq(newjob,cmd);
		#ifdef DEBUG
			printf("Job Status After ENQ\n");
			do_stat(cmd);
		#endif
		enqflag++;
		break;
	case DEQ:
		/*执行DEQ*/
		#ifdef DEBUG
			printf("Execute deq!\n");
		#endif
		#ifdef DEBUG
			printf("Job Status Before DEQ\n");
			do_stat(cmd);
		#endif
		do_deq(cmd);
		#ifdef DEBUG
			printf("Job Status After DEQ\n");
			do_stat(cmd);
		#endif
		break;
	case STAT:
		/*执行STAT*/
		#ifdef DEBUG
			printf("Execute stat!\n");
		#endif
		#ifdef DEBUG
			printf("Job Status Before STAT\n");
			do_stat(cmd);
		#endif
		do_stat(cmd);
		#ifdef DEBUG
			printf("Job Status After STAT\n");
			do_stat(cmd);
		#endif
		break;
	default:
		break;
	}
	#ifdef IMPSEGMT
		printf("timecount: %d\t timeslice: %d\n", timecount, timeslice);
		printf("Status Before Switch job\n");
		do_stat(cmd);
	#endif
	/*需要考虑抢占*/
	if (timecount != timeslice && !enqflag){	/*时间片未到，继续执行*/
		timecount++;
		return;
	}
	else{										/*时间片执行完毕，进行作业切换相关任务*/
		timecount = 1;							/*讲当前运行时间改为1*/
		/* 选择高优先级作业 */
		#ifdef DEBUG
			printf("Select which job to run next!\n");
		#endif
		next=jobselect();
		/*打印执行jobselect选择的进程的信息打印*/
		#ifdef DEBUG
			if (next){
				printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tSTATE\n");
				printf("%d\t%d\t%d\t%d\t%d\t\t%s\n",
					next->job->jid,
					next->job->pid,
					next->job->ownerid,
					next->job->run_time,
					next->job->wait_time,
					"READY");
			}
			else{
				printf("Next is NULL\n");
			}
		#endif
		/* 作业切换 */
		#ifdef DEBUG
			printf("Switch to the next job!\n");
		#endif
		/*作业切换前信息打印*/
		#ifdef DEBUG
			if(current)
				strcpy(timebuf,ctime(&(current->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("/---------- Before jobswitching ----------/\n");
			do_stat(cmd);
			if (next!=NULL){
				printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
					next->job->jid,
					next->job->pid,
					next->job->ownerid,
					next->job->run_time,
					next->job->wait_time,
					timebuf,
					"READY");
			}
		#endif
		jobswitch();
		#ifdef IMPSEGMT
			printf("Status After Switch Job!\n");
			do_stat(cmd);
		#endif
		/*作业切换后作业信息打印*/
		#ifdef DEBUG
			printf("/---------- After jobswitching ----------/\n");
			do_stat(cmd);
		#endif
	}
}

/* 分配作业ID*/
int allocjid()
{

	return ++jobid;
}

void updateall()
{
	struct waitqueue *p;
	struct jobcmd tmpcmd;
	#ifdef DEBUG
		printf("/********** Updateall starts **********/\n");
		do_stat(tmpcmd);
	#endif

	/* 更新作业运行时间 */
	if(current)
		current->job->run_time += 1; /* 加1代表1000ms */

	/* 更新作业等待时间及优先级 */	/*只更新当前任务所属的队列*/
	for(p = head; p != NULL; p = p->next){
		p->job->wait_time += 1000;
		if(p->job->wait_time >= 10000 && p->job->curpri < 3){	//每等待10s优先级+1
			p->job->curpri++;							
			p->job->wait_time = 0;
		}
	}

	#ifdef DEBUG
		printf("/********** Updateall ends **********/\n");
		do_stat(tmpcmd);
	#endif
}

struct waitqueue* jobselect()
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;
	oldhead = head;
	oldhead_id = head_id;
	select = NULL;
	selectprev = NULL;
	/*首先选择当前操作所在队列*/
	#ifdef IMPSEGMT
		printf("In jobselect\n");
		if (!head_1)
			printf("head_1 is NULL\n");
		if (!head_2)
			printf("head_2 is NULL\n");
		if (!head_3)
			printf("head_3 is NULL\n");
	#endif
	if (head_1){
		head = head_1;
		head_id = 1;
	}
	else if (head_2){
		head = head_2;
		head_id = 2;
	}
	else if (head_3){
		head = head_3;
		head_id = 3;
	}
	if (head_id != oldhead_id){					//队列发生切换，改变时间片的值
		timeslice = (head_id == 1)? 1 : (head_id == 2) ? 2 : 5;
	}
	if(head){
		/* 遍历等待队列中的作业，找到优先级最高的作业 */
		for(prev = head, p = head; p != NULL; prev = p,p = p->next)
			if(p->job->curpri > highest){
				select = p;
				selectprev = prev;
				highest = p->job->curpri;
			}
		//if (select != selectprev)
		selectprev->next = select->next;
		if (select == selectprev){
			#ifdef IMPSEGMT
				printf("head = head->next\n");
			#endif
			switch(head_id){
				case 1:
					head_1 = head_1->next;
					break;
				case 2:
					head_2 = head_2->next;
					break;
				case 3:
					head_3 = head_3->next;
					break;
				default :
					break;
			}
			head = head->next;				//bug resoved
		}
	}
	#ifdef IMPSEGMT
		if (!select){
			printf("select is NULL\n");
		}
	#endif
	return select;
}

void jobswitch()
{

	struct waitqueue *p;
	int i;

	if(current && current->job->state == DONE){ /* 当前作业完成 */
		/* 作业完成，删除它 */
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* 释放空间 */
		free(current->job->cmdarg);
		free(current->job);
		free(current);

		current = NULL;
	}

	if(next == NULL && current == NULL) /* 没有作业要运行 */
		return;
	else if (next != NULL && current == NULL){ /* 开始新的作业 */

		printf("begin start new job\n");
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		kill(current->job->pid,SIGCONT);
		return;
	}
	else if (next != NULL && current != NULL){ /* 切换作业并且将需要转换队列的作业进行队列转换 */

		printf("switch to Pid: %d\n",next->job->pid);
		kill(current->job->pid,SIGSTOP);
		current->job->curpri = current->job->defpri;	//切换作业后将作业的当前优先级调节为初始值
		current->job->wait_time = 0;
		current->job->state = READY;
		if (head_id != oldhead_id){		/* 发生队列的切换 */
			struct waitqueue *tmphead;
			if (head_id == 1){					
				tmphead = oldhead;
			}
			else{			
				tmphead = head;
			}
			if (tmphead){
				for (p = tmphead; p->next != NULL;p = p->next);
				p->next = current;
				current->next = NULL;
			}
			else{			//队列头为空, 此时tmphead = oldhead
				if (head_id == 1){
					oldhead = current;
					switch (oldhead_id){
						case 1:
							head_1 = current;
							break;
						case 2:
							head_2 = current;
							break;
						case 3:
							head_3 = current;
							break;
						default :
							break;
					}
				}
				else{
					head = current;
					switch (head_id){
						case 1:
							head_1 = current;
							break;
						case 2:
							head_2 = current;
							break;
						case 3:
							head_3 = current;
							break;
						default :
							break;
					}
				}
				current->next = NULL;
			}
			current = next;
			next = NULL;
			current->job->state = RUNNING;
			current->job->wait_time = 0;
			kill(current->job->pid, SIGCONT);
			return;
		}
		else{						/* 未发生队列的切换*/
			struct waitqueue *tmphead;
			/* 放回等待队列 */
			switch (head_id){
				case 1:
					tmphead = head_2;
					break;
				case 2:
					tmphead = head_3;
					break;
				case 3:
					tmphead = head_3;
					break;
				default:
					break;
			}
			if(tmphead){										//队列不为空
				for(p = tmphead; p->next != NULL; p = p->next);	//找到队尾
				p->next = current;								//将当前工作挂到等待队列
				current->next = NULL;
			}else{
				if (head_id == 1)
					head_2 = current;
				else
					head_3 = current;
				current->next = NULL;
			}
			current = next;
			next = NULL;
			current->job->state = RUNNING;
			current->job->wait_time = 0;
			kill(current->job->pid,SIGCONT);
			return;
		}
		
	}else{ /* next == NULL && current != NULL*/  //bug，第一个程序不显示运行
		if (flag){
			kill(current->job->pid,SIGCONT);
			flag--;
		}
		switch (head_id){
			case 1:
				oldhead = head;
				oldhead_id = head_id;
				head_id = 2;
				head = head_2;
				timeslice = (head_id == 1)? 1 : (head_id == 2) ? 2 : 5;
				break;
			case 2:
				oldhead = head;
				oldhead_id = head_id;
				head_id = 3;
				head = head_3;
				timeslice = (head_id == 1)? 1 : (head_id == 2) ? 2 : 5;
				break;
			case 3:
				oldhead = head;
				oldhead_id = head_id;
				head_id = 3;
				head = head_3;
				timeslice = (head_id == 1)? 1 : (head_id == 2) ? 2 : 5;
				break;
		}
		return;
	}
}

void sig_handler(int sig,siginfo_t *info,void *notused)
{
	int status;
	int ret;
	struct jobcmd tmpcmd;

	switch (sig) {
		case SIGVTALRM: /* 到达计时器所设置的计时间隔 */
			scheduler();
			#ifdef DEBUG
				printf("SIGVTALRM RECEIVED!\n");
			#endif
			return;
		case SIGCHLD: /* 子进程结束时传送给父进程的信号 */
			ret = waitpid(-1,&status,WNOHANG);
			if (ret == 0)
				return;
			if(WIFEXITED(status)){
				current->job->state = DONE;
				printf("normal termation, exit status = %d\n",WEXITSTATUS(status));
			}else if (WIFSIGNALED(status)){
				printf("abnormal termation, signal number = %d\n",WTERMSIG(status));
			}else if (WIFSTOPPED(status)){
				printf("child stopped, signal number = %d\n",WSTOPSIG(status));
			}
			#ifdef DEBUG
				struct waitqueue *p = NULL;
				printf("After child teminated\n");
				printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tSTATE\n");
				if(current){
					printf("%d\t%d\t%d\t%d\t%d\t\t%s\n",
						current->job->jid,
						current->job->pid,
						current->job->ownerid,
						current->job->run_time,
						current->job->wait_time,
						(current->job->state == 0) ? "RUNNING":
						(current->job->state == 1)?"READY":"DONE");
				}

				for(p=head;p!=NULL;p=p->next){
					printf("%d\t%d\t%d\t%d\t%d\t\t%s\n",
						p->job->jid,
						p->job->pid,
						p->job->ownerid,
						p->job->run_time,
						p->job->wait_time,
						"READY");
				}
			#endif
			return;
		default:
			return;
	}
}

void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p;
	int i=0,pid;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;

	sigemptyset(&zeromask);

	/* 封装jobinfo数据结构 */
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
	newjob->wait_time = 0;
	newjob->run_time = 0;
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	offset = enqcmd.data;
	argvec = enqcmd.data;
	while (i < enqcmd.argnum){
		if(*offset == ':'){
			*offset++ = '\0';
			q = (char*)malloc(offset - argvec);
			strcpy(q,argvec);
			arglist[i++] = q;
			argvec = offset;
		}else
			offset++;
	}

	arglist[i] = NULL;

	#ifdef DEBUG

		printf("enqcmd argnum %d\n",enqcmd.argnum);
		for(i = 0;i < enqcmd.argnum; i++)
			printf("parse enqcmd:%s\n",arglist[i]);

	#endif

	/*向等待队列中增加新的作业*/
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next =NULL;
	newnode->job=newjob;
	/*新作业直接加到优先级最高的队列(head_1)队尾*/
	if(head_1)
	{
		for(p=head_1;p->next != NULL; p=p->next);	//找队尾
		p->next =newnode;
	}else
		head_1=newnode;

	/*为作业创建进程*/
	if((pid=fork())<0)
		error_sys("enq fork failed");

	if(pid==0){
		newjob->pid =getpid();
		/*阻塞子进程,等等执行*/
		raise(SIGSTOP);	//raise向当前的进程发送信号，SIGSTOP为停止/阻塞
	#ifdef DEBUG

			printf("begin running\n");
			for(i=0;arglist[i]!=NULL;i++)
				printf("arglist %s\n",arglist[i]);
	#endif

		/*复制文件描述符到标准输出*/
		dup2(globalfd,1);
		/* 执行命令 */
		if(execv(arglist[0],arglist)<0)
			printf("exec failed\n");
		exit(1);
	}else{
		newjob->pid=pid;
	}
}

void do_deq(struct jobcmd deqcmd)
{
	int deqid,i;
	struct waitqueue *p,*prev,*select,*selectprev, *tmp;
	deqid=atoi(deqcmd.data);

	#ifdef DEBUG
		printf("deq jid %d\n",deqid);
	#endif

	/*current jodid==deqid,终止当前作业*/
	if (current && current->job->jid ==deqid){
		printf("teminate current job\n");
		kill(current->job->pid,SIGKILL);
		for(i=0;(current->job->cmdarg)[i]!=NULL;i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i]=NULL;
		}
		free(current->job->cmdarg);
		free(current->job);
		free(current);
		current=NULL;
	}
	else{ /* 或者在等待队列中查找deqid */
		for (i = 1;i <= 3;i++){
			switch (i){
				case 1:
					tmp = head_1;
					break;
				case 2:
					tmp = head_2;
					break;
				case 3:
					tmp = head_3;
					break;
				default :
					break;
			}
			select=NULL;
			selectprev=NULL;
			if(tmp){
				for(prev=tmp,p=tmp;p!=NULL;prev=p,p=p->next)
					if(p->job->jid==deqid){
						select=p;
						selectprev=prev;
						break;
					}
					selectprev->next=select->next;
					if(select==selectprev){
						(i == 1) ? (head_1 = NULL) : 
						(i == 2) ? (head_2 = NULL) : (head_3 = NULL);
					}
			}
			if(select){
				for(i=0;(select->job->cmdarg)[i]!=NULL;i++){
					free((select->job->cmdarg)[i]);
					(select->job->cmdarg)[i]=NULL;
				}
				free(select->job->cmdarg);
				free(select->job);
				free(select);
				select=NULL;
			}
		}
	}
}

void do_stat(struct jobcmd statcmd)
{
	struct waitqueue *p;
	char timebuf[BUFLEN];
	/*
	*打印所有作业的统计信息:
	*1.作业ID
	*2.进程ID
	*3.作业所有者
	*4.作业运行时间
	*5.作业等待时间
	*6.作业创建时间
	*7.作业状态
	*/

	/* 打印信息头部 */
	
	printf("\n--------------------BEGIN--------------------\n");
	
	if(current){
		printf("Print Current Job Status!\n");
		printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING");
	}

	int i;
	for (i = 1;i <= 3;i++){
		printf("Queue_%d Job Status Print\n", i);
		printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
		if (i == 1)
			p = head_1;
		else if( i == 2)
			p = head_2;
		else
			p= head_3;
		for(;p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY");
		}
		putchar('\n');
	}
	printf("--------------------END--------------------\n\n");
}

int main()
{
	struct timeval interval;
	struct itimerval new,old;
	struct stat statbuf;
	struct sigaction newact,oldact1,oldact2;

	#ifdef DEBUG
		printf("DEBUG IS OPEN!\n");
	#endif

	if(stat("/tmp/server",&statbuf)==0){
		/* 如果FIFO文件存在,删掉 */
		if(remove("/tmp/server")<0)
			error_sys("remove failed");
	}

	if(mkfifo("/tmp/server",0666)<0)
		error_sys("mkfifo failed");
	/* 在非阻塞模式下打开FIFO */
	if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");

	/* 建立信号处理函数 */
	newact.sa_sigaction=sig_handler;
	sigemptyset(&newact.sa_mask);
	newact.sa_flags=SA_SIGINFO;
	sigaction(SIGCHLD,&newact,&oldact1);
	sigaction(SIGVTALRM,&newact,&oldact2);

	/* 设置时间间隔为1000毫秒 */
	interval.tv_sec=1;
	interval.tv_usec=0;

	new.it_interval=interval;
	new.it_value=interval;
	setitimer(ITIMER_VIRTUAL,&new,&old);

	while(siginfo==1);

	close(fifo);
	close(globalfd);
	return 0;
}
