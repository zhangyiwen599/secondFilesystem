#ifndef USER_H
#define USER_H

// #include "MemoryDescriptor.h"
// #include "Process.h"
#include "File.h"
#include "INode.h"
#include "FileManager.h"

/*
 * @comment ������Unixv6�� struct user�ṹ��Ӧ�����ֻ�ı�
 * ���������޸ĳ�Ա�ṹ���֣������������͵Ķ�Ӧ��ϵ����:
 */
class User
{
public:
	static const int EAX = 0;	/* u.u_ar0[EAX]�������ֳ���������EAX�Ĵ�����ƫ���� */
	
	/* u_error's Error Code */
	/* 1~32 ����linux ���ں˴����е�/usr/include/asm/errno.h, ����for V6++ */
	enum ErrorCode
	{
		MY_NOERROR	= 0,	/* No error */
		MY_EPERM	= 1,	/* Operation not permitted */
		MY_ENOENT	= 2,	/* No such file or directory */
		MY_ESRCH	= 3,	/* No such process */
		MY_EINTR	= 4,	/* Interrupted system call */
		MY_EIO		= 5,	/* I/O error */
		MY_ENXIO	= 6,	/* No such device or address */
		MY_E2BIG	= 7,	/* Arg list too long */
		MY_ENOEXEC	= 8,	/* Exec format error */
		MY_EBADF	= 9,	/* Bad file number */
		MY_ECHILD	= 10,	/* No child processes */
		MY_EAGAIN	= 11,	/* Try again */
		MY_ENOMEM	= 12,	/* Out of memory */
		MY_EACCES	= 13,	/* Permission denied */
		MY_EFAULT  = 14,	/* Bad address */
		MY_ENOTBLK	= 15,	/* Block device required */
		MY_EBUSY 	= 16,	/* Device or resource busy */
		MY_EEXIST	= 17,	/* File exists */
		MY_EXDEV	= 18,	/* Cross-device link */
		MY_ENODEV	= 19,	/* No such device */
		MY_ENOTDIR	= 20,	/* Not a directory */
		MY_EISDIR	= 21,	/* Is a directory */
		MY_EINVAL	= 22,	/* Invalid argument */
		MY_ENFILE	= 23,	/* File table overflow */
		MY_EMFILE	= 24,	/* Too many open files */
		MY_ENOTTY	= 25,	/* Not a typewriter(terminal) */
		MY_ETXTBSY	= 26,	/* Text file busy */
		MY_EFBIG	= 27,	/* File too large */
		MY_ENOSPC	= 28,	/* No space left on device */
		MY_MY_ESPIPE	= 29,	/* Illegal seek */
		MY_EROFS	= 30,	/* Read-only file system */
		MY_EMLINK	= 31,	/* Too many links */
		MY_EPIPE	= 32,	/* Broken pipe */
		MY_ENOSYS	= 100
		//EFAULT	= 106
	};

	static const int NSIG = 32;	/* �źŸ��� */

	/* p_sig�н��ܵ����źŶ��� */
	static const int SIGNUL = 0;	/* No Signal Received */
	static const int SIGHUP = 1;	/* Hangup (kill controlling terminal) */
	static const int SIGINT = 2;    /* Interrupt from keyboard */
	static const int SIGQUIT = 3;	/* Quit from keyboard */
	static const int SIGILL = 4;	/* Illegal instrution */
	static const int SIGTRAP = 5;	/* Trace trap */
	static const int SIGABRT = 6;	/* use abort() API */
	static const int SIGBUS = 7;	/* Bus error */
	static const int SIGFPE = 8;	/* Floating point exception */
	static const int SIGKILL = 9;	/* Kill(can't be caught or ignored) */
	static const int SIGUSR1 = 10;	/* User defined signal 1 */
	static const int SIGSEGV = 11;	/* Invalid memory segment access */
	static const int SIGUSR2 = 12;	/* User defined signal 2 */
	static const int SIGPIPE = 13;	/* Write on a pipe with no reader, Broken pipe */
	static const int SIGALRM = 14;	/* Alarm clock */
	static const int SIGTERM = 15;	/* Termination */
	static const int SIGSTKFLT = 16; /* Stack fault */
	static const int SIGCHLD = 17; /* Child process has stopped or exited, changed */
	static const int SIGCONT = 18; /* Continue executing, if stopped */
	static const int SIGSTOP = 19; /* Stop executing */
	static const int SIGTSTP = 20; /* Terminal stop signal */
	static const int SIGTTIN = 21; /* Background process trying to read, from TTY */
	static const int SIGTTOU = 22; /* Background process trying to write, to TTY */
	static const int SIGURG = 23;  /* Urgent condition on socket */
	static const int SIGXCPU = 24; /* CPU limit exceeded */
	static const int SIGXFSZ = 25; /* File size limit exceeded */
	static const int SIGVTALRM = 26; /* Virtual alarm clock */
	static const int SIGPROF = 27; /* Profiling alarm clock */
	static const int SIGWINCH = 28; /* Window size change */
	static const int SIGIO = 29; /* I/O now possible */
	static const int SIGPWR = 30; /* Power failure restart */
	static const int SIGSYS = 31; /* invalid sys call */

public:
	// unsigned long u_rsav[2];	/* ���ڱ���esp��ebpָ�� */
	// unsigned long u_ssav[2];	/* ���ڶ�esp��ebpָ��Ķ��α��� */
	// Process* u_procp;			/* ָ���u�ṹ��Ӧ��Process�ṹ */
	// /* ����ӱ������������ԭ�еı���
	//  * int u_uisa[16]
	//  * int u_uisd[16]
	//  * u_tsize
	//  * u_dsize
	//  * u_ssize
	//  */
	// MemoryDescriptor u_MemoryDescriptor;

	/* ϵͳ������س�Ա */
	unsigned int	u_ar0;		/* ָ�����ջ�ֳ���������EAX�Ĵ���
								��ŵ�ջ��Ԫ�����ֶδ�Ÿ�ջ��Ԫ�ĵ�ַ��
								��V6��r0���ϵͳ���õķ���ֵ���û�����
								x86ƽ̨��ʹ��EAX��ŷ���ֵ�����u.u_ar0[R0] */

	int u_arg[5];				/* ��ŵ�ǰϵͳ���ò��� */
	char* u_dirp;				/* ϵͳ���ò���(һ������Pathname)��ָ�� */

	
	
	/* �ļ�ϵͳ��س�Ա */
	Inode* u_cdir;		/* ָ��ǰĿ¼��Inodeָ�� */
	Inode* u_pdir;		/* ָ��Ŀ¼��Inodeָ�� */

	DirectoryEntry u_dent;					/* ��ǰĿ¼��Ŀ¼�� */
	char u_dbuf[DirectoryEntry::DIRSIZ];	/* ��ǰ·������ */
	char u_curdir[128];						/* ��ǰ����Ŀ¼����·�� */
	ErrorCode u_error;
	
	/* �ļ�ϵͳ��س�Ա */
	OpenFiles u_ofiles;		/* ���̴��ļ������������ */

	/* �ļ�I/O���� */
	IOParameter u_IOParam;	/* ��¼��ǰ����д�ļ���ƫ�������û�Ŀ�������ʣ���ֽ������� */

	/* Member Functions */

};

#endif

