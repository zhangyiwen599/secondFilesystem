#ifndef KERNEL_H
#define KERNEL_H


#include "User.h"
#include "BufferManager.h"

#include "FileManager.h"
#include "FileSystem.h"


/*
 * Kernel�����ڷ�װ�����ں���ص�ȫ����ʵ������
 * ����PageManager, ProcessManager�ȡ�
 * 
 * Kernel�����ڴ���Ϊ����ģʽ����֤�ں��з�װ���ں�
 * ģ��Ķ���ֻ��һ��������
 */
class Kernel
{
public:
	static const unsigned long USER_ADDRESS = 0x400000 - 0x1000 + 0xc0000000;	/* 0xC03FF000 */
	static const unsigned long USER_PAGE_INDEX = 1023;		/* USER_ADDRESS��Ӧҳ������PageTable�е����� */

public:
	Kernel();
	~Kernel();
	static Kernel& Instance();
	void Initialize(char * memdisk);		/* �ú�����ɳ�ʼ���ں˴󲿷����ݽṹ�ĳ�ʼ�� */

	
	BufferManager& GetBufferManager();
	
	FileSystem& GetFileSystem();
	FileManager& GetFileManager();
	User& GetUser();		/* ��ȡ��ǰ���̵�User�ṹ */

private:
	void InitBuffer(char *memdisk);
	void InitFileSystem();

private:
	static Kernel instance;		/* Kernel������ʵ�� */

	
	BufferManager* m_BufferManager;

	FileSystem* m_FileSystem;
	FileManager* m_FileManager;
	User * m_User;
};

#endif
