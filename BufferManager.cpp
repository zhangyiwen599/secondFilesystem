#include "BufferManager.h"
#include "Kernel.h"
#include "Utility.h"
#include <string>
using namespace std;

BufferManager::BufferManager()
{
	//nothing to do here
}

BufferManager::~BufferManager()
{
	//nothing to do here
}

//��ʼ��������ƿ�
void BufferManager::Initialize(char *p)
{
	int i;
	Buf* bp;

	this->memdisk = p;

	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);
	// this->bFreeList.av_forw = this->bFreeList.av_back = &(this->bFreeList);

	//��ÿ��������ƿ�ͻ������ݵ�ַ��Ӧ
	for(i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);
		//bp->b_dev = -1;
		bp->b_addr = this->Buffer[i];
		/* ��ʼ��NODEV���� */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		this->bFreeList.b_forw->b_back = bp;
		this->bFreeList.b_forw = bp;
		/* ��ʼ�����ɶ��� */
		bp->b_flags = Buf::B_BUSY;
		Brelse(bp);
	}
	// this->m_DeviceManager = &Kernel::Instance().GetDeviceManager();
	
	return;
}

//��ȡ
Buf* BufferManager::GetBlk(int blkno)
{
	Buf* bp;
	// User& u = Kernel::Instance().GetUser();


	//���Ȳ鿴�Ƿ��л�������Ѵ�����Ҫ�Ĵ�������
	for(bp = this->bFreeList.b_forw; bp!= &this->bFreeList ; bp = bp->b_forw)
	{
		if(bp->b_blkno == blkno)
		{
			bp->b_flags |= Buf::B_BUSY;
			//getLog().write(Log::INFO,string("find buffer in bFreeList, return blkno value:")+to_string(blkno));
			return bp;
		}
	}


	//�����ǵ����̣�����ֱ��ȡ��һ������鼴��
	bp = this->bFreeList.b_forw;

	/* ������ַ������ӳ�д��д�������� */
	if(bp->b_flags & Buf::B_DELWRI)
	{
		// bp->b_flags |= Buf::B_ASYNC;
		// �����ǵ����� ������Ҫ�첽д
		this->Bwrite(bp);
	}
	/* ע��: �����������������λ��ֻ����B_BUSY */
	bp->b_flags = Buf::B_BUSY;

	/* ��ԭ�豸�����г�� */
	bp->b_back->b_forw = bp->b_forw;
	bp->b_forw->b_back = bp->b_back;
	/* ����ԭ����β�� */
	bp->b_forw = &this->bFreeList;
	this->bFreeList.b_back->b_forw = bp;
	bp->b_back = this->bFreeList.b_back;
	this->bFreeList.b_back = bp;

	
	bp->b_blkno = blkno;
	//getLog().write(Log::INFO,string("get free buf"));
	return bp;
}

void BufferManager::Brelse(Buf* bp)
{
	
	
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	// (this->bFreeList.av_back)->av_forw = bp;
	// bp->av_back = this->bFreeList.av_back;
	// bp->av_forw = &(this->bFreeList);
	// this->bFreeList.av_back = bp;
	//������ֻ��һ�����У���Щ��������Ҫ
	
	
	return;
}



Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	/* �����豸�ţ��ַ�������뻺�� */
	bp = this->GetBlk(blkno);
	/* ������豸�������ҵ����軺�棬��B_DONE�����ã��Ͳ������I/O���� */
	if(bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* û���ҵ���Ӧ���棬����I/O������� */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;

	/* 
	 * ��I/O�����������Ӧ�豸I/O������У���������I/O����������ִ�б���I/O����
	 * ����ȴ���ǰI/O����ִ����Ϻ����жϴ����������ִ�д�����
	 * ע��Strategy()������I/O����������豸������к󣬲���I/O����ִ����ϣ���ֱ�ӷ��ء�
	 */
	// this->m_DeviceManager->GetBlockDevice(Utility::GetMajor(dev)).Strategy(bp);
	// /* ͬ�������ȴ�I/O�������� */
	// this->IOWait(bp);

	memcpy(bp->b_addr,(memdisk + BufferManager::BUFFER_SIZE*blkno),BufferManager::BUFFER_SIZE);
	//�����Ƕ����ļ�ϵͳ������ֱ�Ӵ��ڴ��ж�ȡ���ݼ���
	//getLog().write(Log::INFO,string("read from disk success"));

	return bp;
}



void BufferManager::Bwrite(Buf *bp)
{
	unsigned int flags;

	flags = bp->b_flags;
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512�ֽ� */

	

	if( (flags & Buf::B_ASYNC) == 0 )
	{
		/* ͬ��д����Ҫ�ȴ�I/O�������� */
		memcpy((memdisk + BufferManager::BUFFER_SIZE*bp->b_blkno),bp->b_addr,BufferManager::BUFFER_SIZE);
	}
	else if( (flags & Buf::B_DELWRI) == 0)
	{
	/* 
	 * ��������ӳ�д��������󣻷��򲻼�顣
	 * ������Ϊ����ӳ�д������п��ܵ�ǰ���̲���
	 * ������һ�����Ľ��̣�����GetError()��Ҫ��
	 * ����ǰ���̸��ϴ����־��
	 */
		memcpy((memdisk + BufferManager::BUFFER_SIZE*bp->b_blkno),bp->b_addr,BufferManager::BUFFER_SIZE);
	}
	this->Brelse(bp);
	//getLog().write(Log::INFO,string("write to disk success"));
	return;
}

void BufferManager::Bdwrite(Buf *bp)
{
	/* ����B_DONE������������ʹ�øô��̿����� */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

void BufferManager::Bawrite(Buf *bp)
{
	/* ���Ϊ�첽д */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* ������������������ */
	for(unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	//getLog().write(Log::INFO,string("clear buf success"));

	return;
}

void BufferManager::Bflush()
{
	Buf* bp;
	/* ע�⣺����֮����Ҫ��������һ����֮�����¿�ʼ������
	 * ��Ϊ��bwite()���뵽����������ʱ�п��жϵĲ���������
	 * �ȵ�bwriteִ����ɺ�CPU�Ѵ��ڿ��ж�״̬�����Ժ�
	 * �п��������ڼ���������жϣ�ʹ��bfreelist���г��ֱ仯��
	 * �����������������������������¿�ʼ������ô�ܿ�����
	 * ����bfreelist���е�ʱ����ִ���
	 */

	for(bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
		/* �ҳ����ɶ����������ӳ�д�Ŀ� */
		if( (bp->b_flags & Buf::B_DELWRI) )
		{
			/* ��ԭ�豸�����г�� */
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;
			/* ����ԭ����β�� */
			bp->b_forw = &this->bFreeList;
			this->bFreeList.b_back->b_forw = bp;
			bp->b_back = this->bFreeList.b_back;
			this->bFreeList.b_back = bp;

			this->Bwrite(bp);
		}
	}

	return;
}



Buf* BufferManager::InCore(int blkno)
{
	Buf* bp;
	
	
	for(bp = this->bFreeList.b_forw; bp !=&this->bFreeList ; bp = bp->b_forw)
	{
		if(bp->b_blkno == blkno)
			return bp;
	}
	return NULL;
}

Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

