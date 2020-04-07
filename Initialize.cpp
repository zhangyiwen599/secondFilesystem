#include "FileSystem.h"
#include "OpenFileManager.h"
#include "Kernel.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include "SystemCall.h"
using namespace std;

const char *disk_file_name = "c.img";


void init_blocks()
{
    cout << "Initialize Blocks" <<endl;
    g_spb.s_isize = FileSystem::INODE_ZONE_SIZE;
    g_spb.s_fsize = FileSystem::DATA_ZONE_END_SECTOR + 1;
    // 第一组是99块，后面都是100块 剩下由超级块直接管理
    //s.s_nfree = (FileSystem::DATA_ZONE_SIZE -99)%100;
    FileSystem filesys = Kernel::Instance().GetFileSystem();


    /* 
	 * 对数据区( 128 <= blkno < 1023 )中每个扇区，Free(blkno)一下，
	 * 即可将所有free block按照"栈的栈"方式组织起来。
	 */
    // 此处借鉴了Unixv6++的写法
    g_spb.s_nfree = 0;
    for (int blkno = FileSystem::DATA_ZONE_END_SECTOR; blkno >= FileSystem::DATA_ZONE_START_SECTOR; --blkno)
    {
        filesys.Free(blkno);
    }
    //循环完毕后 s_nfree 以及 s_free[] 就都完成了初始化
    cout<<g_spb.s_nfree <<endl;
    
    /* Init spb.s_inode[]; */
	int total_inode = FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR;
	
	g_spb.s_ninode = 0;
	for(int inode_num = total_inode - 1, count = 31; count > 0; --count )
	{
		g_spb.s_inode[g_spb.s_ninode++] = inode_num;
		inode_num--;
	}
    // cout << 3 << endl;
    /* 0# DiskInode permanently for root dir */
	g_spb.s_inode[g_spb.s_ninode++] = 0;    //在栈顶放 0# DiskInode
    //cout << 4 << endl;

    /****************** 将0# DiskInode分配做根目录的DiskInode ******************/
    Inode* pNode = filesys.IAlloc();

    //cout << 5 << endl;

    User& u = Kernel::Instance().GetUser();
    pNode->i_flag |= (Inode::IACC | Inode::IUPD);
	pNode->i_mode = Inode::IALLOC | Inode::IFDIR /* Most vital!! */| Inode::IREAD | Inode::IWRITE | Inode::IEXEC | (Inode::IREAD >> 3) | (Inode::IWRITE >> 3) | (Inode::IEXEC >> 3) | (Inode::IREAD >> 6) | (Inode::IWRITE >> 6) | (Inode::IEXEC >> 6);
    pNode->i_nlink  = 1;
    

    g_InodeTable.IPut(pNode);	/* 将rootDir DiskInode写入磁盘 */
    /****************** 将0# DiskInode分配做根目录的DiskInode END ******************/
     
    g_spb.s_ronly = 0;
    g_spb.s_fmod = 1;

    //将super block所有信息写入磁盘
    filesys.Update();


    cout << "Initialize Block success" <<endl;
}


void spb_init(SuperBlock &sb)
{
	sb.s_isize = FileSystem::INODE_ZONE_SIZE;
	sb.s_fsize = FileSystem::DATA_ZONE_END_SECTOR+1;

	//第一组99块 其他都是一百块一组 剩下的被超级快直接管理
	sb.s_nfree = (FileSystem::DATA_ZONE_SIZE - 99) % 100;

	//超级快直接管理的空闲盘块的第一个盘块的盘块号
	//成组链表法
	int start_last_datablk = FileSystem::DATA_ZONE_START_SECTOR;
	for (;;)
		if ((start_last_datablk + 100 - 1) < FileSystem::DATA_ZONE_END_SECTOR)//判断剩下盘块是否还有100个
			start_last_datablk += 100;
		else
			break;
	start_last_datablk--;
	for (int i = 0; i < sb.s_nfree; i++)
		sb.s_free[i] = start_last_datablk + i;

	sb.s_ninode = 100;
	for (int i = 0; i < sb.s_ninode; i++)
		sb.s_inode[i] = i ;//注：这里只是diskinode的编号，真正取用的时候要进行盘块的转换

	sb.s_fmod = 0;
	sb.s_ronly = 0;
	
	
}

void init_datablock(char *data)
{
	struct {
		int nfree;//本组空闲的个数
		int free[100];//本组空闲的索引表
	}tmp_table;

	int last_datablk_num = FileSystem::DATA_ZONE_SIZE;//未加入索引的盘块的数量
	//注:成组连接法,必须的初始化索引表
	for (int i = 0;; i++)
	{
		if (last_datablk_num >= 100)
			tmp_table.nfree = 100;
		else
			tmp_table.nfree = last_datablk_num;
		last_datablk_num -= tmp_table.nfree;

		for (int j = 0; j < tmp_table.nfree; j++)
		{
			if (i == 0 && j == 0)
				tmp_table.free[j] = 0;
			else
			{
				tmp_table.free[j] = 100 * i + j + FileSystem::DATA_ZONE_START_SECTOR - 1;
			}
		}
		memcpy(&data[i * 100 * 512], (void*)&tmp_table, sizeof(tmp_table));
		if (last_datablk_num == 0)
			break;
	}
}

int init_img(int fd)
{
	SuperBlock spb;
	spb_init(spb);
	DiskInode *di = new DiskInode[FileSystem::INODE_ZONE_SIZE*FileSystem::INODE_NUMBER_PER_SECTOR];

	//设置rootDiskInode的初始值
	di[0].d_mode = Inode::IFDIR;
	di[0].d_mode |= Inode::IEXEC;
	//di[0].d_nlink = 888;

	char *datablock = new char[FileSystem::DATA_ZONE_SIZE * 512];
	memset(datablock, 0, FileSystem::DATA_ZONE_SIZE * 512);
	init_datablock(datablock);

	write(fd, &spb,  sizeof(SuperBlock));
	write(fd, di, FileSystem::INODE_ZONE_SIZE*FileSystem::INODE_NUMBER_PER_SECTOR * sizeof(DiskInode));
	write(fd, datablock, FileSystem::DATA_ZONE_SIZE * 512);

	// cout << "格式化磁盘完毕" << endl;
//	exit(1);
}


int main()
{
    int fd = open(disk_file_name, O_RDWR);
    
	
	
    if (fd == -1)
    {
        cout << "img file isn't exist,create a new one " << endl;
        fd = open(disk_file_name, O_RDWR| O_CREAT, 0666);
        if (fd == -1)
        {
            cout << "create img file failed" << endl;
            exit(0);
        }
        init_img(fd);
        
        
        cout << "new img file create success"<<endl;
    }
    else{
        cout << "open img file success" <<endl;
    }
    struct stat file_st;
	fstat(fd,&file_st);
  	int len = file_st.st_size;
   
    /*把文件映射成虚拟内存地址*/
    void *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED,fd , 0);

    if ((void *)-1 == addr)
    {
        printf("map file error: %s\n", strerror(errno));
        exit(-1);
    }
    Kernel::Instance().Initialize( (char *)addr);
    // init_blocks();
	init_sys();

	my_mkdir("etc");
	my_mkdir("bin");
	my_mkdir("home");
	my_mkdir("dev");

    //取消映射，关闭文件
   	exitOS((char *)addr,len);
    close(fd);
}
