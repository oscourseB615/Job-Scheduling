# Job-Scheduling
### 需求说明
#### 基本要求和提高要求
* 基本要求：学习源代码，并完成十个调试任务。
* 提高要求：实现多级反馈轮转调度算法
	* 多级——多个队列。
	* 反馈——优先级会根据等待时间变化
	* 轮转——同优先级的作业轮流进行
* 具体要求：
要求实现3个队列，每个队列时间片不一样：最高优先级队列的轮转时间为1s，次高优先级为2s，最低优先级为5s；当有新作业加入，抢占式>运行，而不是等待当前时间片结束；高优先级队列结束前，不轮转低优先级队列，等待时间超过10s自动升高优先级一级。


#### 完成情况
* 基本要求：十个调试任务
	*	任务1：在`job.c`的main函数中，所有的struct声明之后，添加调试代码
</br></br>
![调试1](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.1.png)
</br></br>
	* 	任务2：定时处理函数`sig_handler()`调试
</br></br>
![调试2](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.2.png)
</br></br>
	* 	任务3：调度程序`scheduler()`调试
</br></br>
![调试3](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.3.png)
</br></br>
	* 	任务4：作业入队命令调试
</br></br>
![调试4](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.4.png)
</br></br>
	* 	任务5：DEQ命令和STAT命令调试
</br></br>
![调试5](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.5.png)
</br></br>
	* 	任务6：`updateall`函数调试
</br></br>
![调试6](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.6.png)
</br></br>
	* 任务7：在`job.c`里，实现“显示ENQ，DEQ，STAT命令执行前后的队列里所有进程的相关信息”
		* ENQ：
</br></br>
![调试7.ENQ](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.7.ENQ.png)
</br></br>
		* DEQ
</br></br>
![调试7.DEQ](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.7.DEQ.png)
</br></br>
		* STAT：
</br></br>
![调试7.STAT](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.7.STAT.png)
</br></br>
	* 	任务8：jobselect函数调试：在`job.c`里，实现“显示`jobselect`函数选择的进程的信息”
</br></br>
![调试8](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.8.png)
</br></br>
	* 任务9：`jobswitch`函数调试：在`job.c`里，实现“显示`jobswitch`执行前后的正在执行的进程和等待队列中的分别的情况”
</br></br>
![调试9](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.9.png)
</br></br>
	* 任务10：
</br></br>
![调试10](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/Debug.10.png)
</br></br>
* 提高部分：完成，但是当时我自己的理解与原提高要求有出入，主要是：1. 入队的时候我都是默认将任务加到优先级最高的队伍，2. 对于初始给定优先级的作业，其优先级一直会存在，即时间片用光后会把它的优先级置为初始值。当时助教说需要在实验报告里面说明一下，虽然我和要求有出入，但是对于多级轮转反馈部分也都实现了（只不过差异在具体的细节之上）。
### 设计说明
#### 流程示意图
</br></br>
![绘图](https://raw.githubusercontent.com/oscourseB615/Job-Scheduling/master/images/%E7%BB%98%E5%9B%BE1.png)
</br></br>
#### 所使用的系统调用的列表
|系统调用名称|功能简介|
|----------|-------|
|Fork()	|创建进程|
|Signal()|	产生信号处理句柄|
|Free()	|释放数据结构|
|Kill()	|杀死进场|
|Execv()|	装入并运行函数|
|Open()	|打开文件|
|Close()	|关闭文件|
|Read()	|读文件|
|Write()	|写文件|
|Mkfifo()|	创建FIFO文件|
|Remove()	|移除文件|
|Dup2()	|复制现存的文件描述符|
|Setitimer()|	进行计时|
|Sigaction()|	查询或设置信号处理方式|


### 仓库描述
* **仓库用途** `作业调度/OS实验2` </br>
* **创建日期** `2015/4/18 22:24` </br>
