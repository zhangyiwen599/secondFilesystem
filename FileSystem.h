#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "INode.h"
#include "Buf.h"
#include "BufferManager.h"

/*
 * �ļ�ϵͳ�洢��Դ�����(Super Block)�Ķ��塣
 */
class SuperBlock
{
	/* Functions */
public:
	/* Constructors */
	SuperBlock();
	/* Destructors */
	~SuperBlock();
	
	/* Members */
public:
	int		s_isize;		/* ���Inode��ռ�õ��̿��� */
	int		s_fsize;		/* �̿����� */
	
	int		s_nfree;		/* ֱ�ӹ���Ŀ����̿����� */
	int		s_free[100];	/* ֱ�ӹ���Ŀ����̿������� */
	
	int		s_ninode;		/* ֱ�ӹ���Ŀ������Inode���� */
	int		s_inode[100];	/* ֱ�ӹ���Ŀ������Inode������ */
	
	//������ֻ��һ������ ����Ҫ��
	// int		s_flock;		/* ���������̿��������־ */
	// int		s_ilock;		/* ��������Inode���־ */
	
	int		s_fmod;			/* �ڴ���super block�������޸ı�־����ζ����Ҫ��������Ӧ��Super Block */
	int		s_ronly;		/* ���ļ�ϵͳֻ�ܶ��� */
	int		s_time;			/* ���һ�θ���ʱ�� */
	int		padding[47];	/* ���ʹSuperBlock���С����1024�ֽڣ�ռ��2������ */
	//int padding[49];	//����ȥ������ ����2
	char a[8];
};





/*
 * �ļ�ϵͳ��(FileSystem)�����ļ��洢�豸��
 * �ĸ���洢��Դ�����̿顢���INode�ķ��䡢
 * �ͷš�
 */
class FileSystem
{
public:
	/* static consts */
	// static const int NMOUNT = 5;			/* ϵͳ�����ڹ������ļ�ϵͳ��װ������� */

	// static const int SUPER_BLOCK_SECTOR_NUMBER = 200;	/* ����SuperBlockλ�ڴ����ϵ������ţ�ռ��200��201���������� */
	
	static const int SUPER_BLOCK_SECTOR_NUMBER = 1;	/* ����0����������1��ʼ */

	static const int ROOTINO = 0;			/* �ļ�ϵͳ��Ŀ¼���Inode��� */

	// ���inode�� 3-127
	static const int INODE_NUMBER_PER_SECTOR = 8;		/* ���INode���󳤶�Ϊ64�ֽڣ�ÿ�����̿���Դ��512/64 = 8�����Inode */
	static const int INODE_ZONE_START_SECTOR = 3;		/* ���Inode��λ�ڴ����ϵ���ʼ������ */
	static const int INODE_ZONE_SIZE = 128-3  /*1024 - 202*/;	/* ����Ϊʵ�����ʲ���Ҫ��ô��Inode */	/* ���������Inode��ռ�ݵ������� */

	// ��������� 128-1023
	static const int DATA_ZONE_START_SECTOR = 128;/*1024*/		/* ����������ʼ������ */
	static const int DATA_ZONE_END_SECTOR = 1024 - 1/* 18000 - 1 */;	/* �������Ľ��������� */
	static const int DATA_ZONE_SIZE = 1024 - DATA_ZONE_START_SECTOR;	/* ������ռ�ݵ��������� */

	/* Functions */
public:
	/* Constructors */
	FileSystem();
	/* Destructors */
	~FileSystem();

	/* 
	 * @comment ��ʼ����Ա����
	 */
	void Initialize();

	/* 
	* @comment ϵͳ��ʼ��ʱ����SuperBlock
	*/
	void LoadSuperBlock();

	/* 
	 * @comment ����ֻ��һ���豸������Ҫ�豸��
	 * ���ļ�ϵͳ��SuperBlock
	 */
	SuperBlock* GetFS();


	/* 
	 * @comment ��SuperBlock������ڴ渱�����µ�
	 * �洢�豸��SuperBlock��ȥ
	 */
	void Update();

	/* 
	 * @comment  �ڴ洢�豸�Ϸ���һ������
	 * ���INode��һ�����ڴ����µ��ļ���
	 */
	Inode* IAlloc();


	/* 
	 * @comment  �ͷű��Ϊnumber
	 * �����INode��һ������ɾ���ļ���
	 */
	void IFree(int number);

	/* 
	 * @comment �ڴ洢�豸�Ϸ�����д��̿�
	 */
	Buf* Alloc();

	
	/* 
	 * @comment �ͷŴ洢�豸�ϱ��Ϊblkno�Ĵ��̿�
	 */
	void Free(int blkno);

	/* 
	 * @comment �����ļ�ϵͳװ�������ָ��Inode��Ӧ��Mountװ���
	 */
	// Mount* GetMount(Inode* pInode);

private:
	/* 
	 * @comment ����豸dev�ϱ��blkno�Ĵ��̿��Ƿ�����
	 * �����̿���
	 */
	bool BadBlock(SuperBlock* spb, int blkno);

	/* Members */
public:
	// Mount m_Mount[NMOUNT];		/* �ļ�ϵͳװ����Mount[0]���ڸ��ļ�ϵͳ */

private:
	BufferManager* m_BufferManager;		/* FileSystem����Ҫ�������ģ��(BufferManager)�ṩ�Ľӿ� */
	// int updlock;				/* Update()�����������ú�������ͬ���ڴ����SuperBlock�����Լ���
	// 							���޸Ĺ����ڴ�Inode����һʱ��ֻ����һ�����̵��øú��� */
};
extern SuperBlock g_spb;
#endif
