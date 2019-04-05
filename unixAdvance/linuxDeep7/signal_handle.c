/**
 * 继续不动笔墨不读书系列
 * 掌握信号处理，稳健 处理信号中断和异常
 * 异步的一个方式
 * Linux信号处理底层细节，利用系统的各种信号
 * 信号的基本概念
 * 信号是操作系统中当某个事件发生时对进程的通知机制。当进程接收到信号时，如果存在该信号的处理程序，进程就会被中断
 * 转而去执行指定的信号处理程序
 * 信号因收到信号而被中断的时机是不可预料的，对信号处理与硬件很像
 * 软件中断
 * 信号，用作进程间一种原始的同步或通信手段
 * 常见信号
 * kill -l 可以看到多个普通的信号
 * SIGSEGV 空指针引用、数组越界等导致出现错误的地址值，野指针访问到了没有映射的地址
 *         或试图更新没有写权限的内存页等情况下会引发该信号
 * SIGBUS 试图访问的内存地址没有按照指定的要求对齐时就会引发该信号，常见移动设备
 * SIGFPE 算数运算错误所引发的信号，最常见的如除零引起的异常
 * SIGINT 当再Shell中用Control+C终止一个没有反应的进程时，就会向该进程发送这个中断信号
 * SIGKILL 该信号的编号为9。kill -9命令，向目标进程发送该信号，该信号的默认处理方式是终止进程，而且该行为不能被修改，所以它总是能终止一个运行中的进程，除了僵尸进程和1号进程(init)
 * SIGTRAP 当使用GDB单步或断点调试的时候，利用的就是这个信号 
 * SIGILL 某些错误的C++使用方式，会导致编译器生成一条无效指令，当执行到该条指令时，就会引发一个SIGILL信号，导致程序终止
 * SIGALARM 当使用setitimer设定的定时器到期时，进程就会收到这样一个那种信号
 * 
 * Linux 系统对信号的处理方式大体可分为以下几种
 * 终止进程 
 * 终止进程并产生核心转储文件(Core Dump)
 * 执行用户自定义的信号处理函数
 * 忽略该信息
 * 
 * kill并不只是终止，而是可以进程间发送信号的命令
 * 游戏 信号触发脚本和配置文件的重新加载，实现不停服的游戏数据更新
 * google的性能测试工具gperftools利用SIGALARM 实现对运行中的进程执行位置的周期性采样，实现统计方式发现性能热点的功能
 * 
 * 信号处理流程
 * 指定信号传递给进程时，内核就会打断进程主程序的执行，并代表进程来执行一段信号处理程序
 * 当信号处理程序运行结束后，内核会将被打断的进程从被打断的位置恢复执行
 * 
 * 信号处理程序的函数有两个
 * sighandler_t signal(int signum,sighandler_t handler);
 * int sigaction(int signum,const struct sigaction *act,struct sigaction *oldact);
 * 
 * 为指定信号安装一个信号处理函数，并返回旧的处理函数已备有用。
 * sighandler_t是一个函数指针
 * typedef void (*sighandler_t)(int);
 * 
 * sigaction功能强大一些，它可以对安装的信号处理程序执行更加精细的控制
 * 如果指定了SA_SIGINFO标志，在信号发生时还能获取到关于信号的额更多信息 发送信号进程ID、发送者ID、引发信号产生的指令地址等
 * 
 * ***/
#include <signal.h>

struct sigaction {
   void     (*sa_handler)(int);
   void     (*sa_sigaction)(int, siginfo_t *, void *);
   sigset_t   sa_mask;
   int        sa_flags;
   void     (*sa_restorer)(void);
};

/**
 * 信号处理程序的执行也需要时间，但这段时间又产生了新的信号怎么破？
 * 信号处理程序自己引发信号怎么办？
 * 一个进程接收到信号的频率超过了自己的信号处理程序的处理能力会发生什么？
 * 
 * 两个概念 可重入与异步信号安全
 * 可重入函数 SUSv3 可重入函数的定义为
 * 当函数由两条或多条执行路径调用时，即使交叉执行，其效果也与各执行路径以未定义的顺序依次执行时一致，那么该函数就是可重入函数
 * 换句话说，如果同一个函数不管被外界以怎样的顺序交叉调用，每个调用者都能获得正确的预期结果 而不会互相干扰，那么该函数就是可重入的
 * 
 * 只用到了函数内的局部变量的函数一定是可重入的，如果包含全局或静态数据结构的更新，那么该函数就很可能不是可重入的
 * 特别注意 执行路径 不是线程 
 * 中断处理程序和线程都定义了不同的执行路径，但是它们两者有很大的区别
 * 
 * 信号处理函数需要是可重入的
 * malloc与free，内部更新一个全局的内存块的链表，所以不可重入
 * malloc不能在信号处理函数中使用
 * malloc和free 线程安全
 * 信号处理程序中使用锁是没用的，因为信号处理程序虽然定义的是另外一条程序执行路径，但是它与主程序之间实际还是串行的执行关系
 * 
 * 异步信号安全 设置信号掩码 不被信号中断
 * 可重入函数一定是异常信号安全的，同时还有些函数虽然不可重入，但是如果设置了不能被信号处理器打断，那她也是异常安全的
 * 掩码对应的信号会被内核阻塞而暂时不发送给进程，直到该信号的掩码被移除时为止
 * int sigpromask(int how,const sigset_t *set,sigset_t *oldset);
 * 
 * 信号处理程序需要注意
 * 让信号处理函数可重入 信号异步安全 阻塞相关信号传递，防止信号中断引发错误
 * 任何在主程序与信号处理程序中共享的变量都应该用volatile关键字修饰
 * 原子性保证指令正常执行不被打断
 * volatile sig_atomic_t variable;
 * 
 * ***/
volatile sig_atomic_t variable;

/**
 * 信号处理程序经常被作为程序中处理异常状况的手段，已尽量保证程序的稳定性
 * 或者在异常出现时能准确记录异常出现的状态
 * 
 * 进程的内核栈超过系统限制 产生SIGSEGV信号 自己程序的SIGSEGV信号的处理函数却不执行
 * 
 * 这种情况下 可以预先为信号处理程序准备一块预留好的内存区，作为信号处理函数的备选栈
 * sigaltstack 准备预留区 SA_OBSTACK标志
 * 
 * 信号对阻塞的系统调用的中断
 * 
 * 所以信号对于异步安全和可重入是有要求的，信号不会打断程序就能成为信号安全的
 * 但是系统调用也会中断程序，所以，一般都要对这个进行处理，进行重试，处理errno
 * 信号处理程序小心处理全局数据
 * ***/