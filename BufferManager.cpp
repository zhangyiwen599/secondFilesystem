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

//初始化缓冲控制块
void BufferManager::Initialize(char *p)
{
	int i;
	Buf* bp;

	this->memdisk = p;

	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);
	// this->bFreeList.av_forw = this->bFreeList.av_back = &(this->bFreeList);

	//将每个缓存控制块和缓存数据地址对应
	for(i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);
		//bp->b_dev = -1;
		bp->b_addr = this->Buffer[i];
		/* 初始化NODEV队列 */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		this->bFreeList.b_forw->b_back = bp;
		this->bFreeList.b_forw = bp;
		/* 初始化自由队列 */
		bp->b_flags = Buf::B_BUSY;
		Brelse(bp);
	}
	// this->m_DeviceManager = &Kernel::Instance().GetDeviceManager();
	
	return;
}

//获取
Buf* BufferManager::GetBlk(int blkno)
{
	Buf* bp;
	// User& u = Kernel::Instance().GetUser();


	//首先查看是否有缓存块上已存在需要的磁盘扇区
	for(bp = this->bFreeList.b_forw; bp!= &this->bFreeList ; bp = bp->b_forw)
	{
		if(bp->b_blkno == blkno)
		{
			bp->b_flags |= Buf::B_BUSY;
			//getLog().write(Log::INFO,string("find buffer in bFreeList, return blkno value:")+to_string(blkno));
			return bp;
		}
	}


	//由于是单进程，所以直接取第一个缓存块即可
	bp = this->bFreeList.b_forw;

	/* 如果该字符块是延迟写，写到磁盘上 */
	if(bp->b_flags & Buf::B_DELWRI)
	{
		// bp->b_flags |= Buf::B_ASYNC;
		// 由于是单进程 ，不需要异步写
		this->Bwrite(bp);
	}
	/* 注意: 这里清除了所有其他位，只设了B_BUSY */
	bp->b_flags = Buf::B_BUSY;

	/* 从原设备队列中抽出 */
	bp->b_back->b_forw = bp->b_forw;
	bp->b_forw->b_back = bp->b_back;
	/* 放入原队列尾部 */
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
	//单进程只有一个队列，这些操作不需要
	
	
	return;
}



Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	/* 根据设备号，字符块号申请缓存 */
	bp = this->GetBlk(blkno);
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if(bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* 没有找到相应缓存，构成I/O读请求块 */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;

	/* 
	 * 将I/O请求块送入相应设备I/O请求队列，如无其它I/O请求，则将立即执行本次I/O请求；
	 * 否则等待当前I/O请求执行完毕后，由中断处理程序启动执行此请求。
	 * 注：Strategy()函数将I/O请求块送入设备请求队列后，不等I/O操作执行完毕，就直接返回。
	 */
	// this->m_DeviceManager->GetBlockDevice(Utility::GetMajor(dev)).Strategy(bp);
	// /* 同步读，等待I/O操作结束 */
	// this->IOWait(bp);

	memcpy(bp->b_addr,(memdisk + BufferManager::BUFFER_SIZE*blkno),BufferManager::BUFFER_SIZE);
	//由于是二级文件系统，所以直接从内存中读取数据即可
	//getLog().write(Log::INFO,string("read from disk success"));

	return bp;
}



void BufferManager::Bwrite(Buf *bp)
{
	unsigned int flags;

	flags = bp->b_flags;
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512字节 */

	

	if( (flags & Buf::B_ASYNC) == 0 )
	{
		/* 同步写，需要等待I/O操作结束 */
		memcpy((memdisk + BufferManager::BUFFER_SIZE*bp->b_blkno),bp->b_addr,BufferManager::BUFFER_SIZE);
	}
	else if( (flags & Buf::B_DELWRI) == 0)
	{
	/* 
	 * 如果不是延迟写，则检查错误；否则不检查。
	 * 这是因为如果延迟写，则很有可能当前进程不是
	 * 操作这一缓存块的进程，而在GetError()主要是
	 * 给当前进程附上错误标志。
	 */
		memcpy((memdisk + BufferManager::BUFFER_SIZE*bp->b_blkno),bp->b_addr,BufferManager::BUFFER_SIZE);
	}
	this->Brelse(bp);
	//getLog().write(Log::INFO,string("write to disk success"));
	return;
}

void BufferManager::Bdwrite(Buf *bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

void BufferManager::Bawrite(Buf *bp)
{
	/* 标记为异步写 */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* 将缓冲区中数据清零 */
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
	/* 注意：这里之所以要在搜索到一个块之后重新开始搜索，
	 * 因为在bwite()进入到驱动程序中时有开中断的操作，所以
	 * 等到bwrite执行完成后，CPU已处于开中断状态，所以很
	 * 有可能在这期间产生磁盘中断，使得bfreelist队列出现变化，
	 * 如果这里继续往下搜索，而不是重新开始搜索那么很可能在
	 * 操作bfreelist队列的时候出现错误。
	 */

	for(bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
		/* 找出自由队列中所有延迟写的块 */
		if( (bp->b_flags & Buf::B_DELWRI) )
		{
			/* 从原设备队列中抽出 */
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;
			/* 放入原队列尾部 */
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

