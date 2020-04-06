#include "FileSystem.h"
#include "OpenFileManager.h"
#include "Kernel.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
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

void init_data_block(char *datablock)
{
    // //循环 找出由超级快直接控制的第一个盘块号
    // int start = FileSystem::DATA_ZONE_START_SECTOR;
    // while(1)
    // {
    //     // 由于第一组为99，所以判断条件为99
    //     if(start + 99 < FileSystem::DATA_ZONE_END_SECTOR)
    //     {
    //         start += 100;
    //     }
    //     else
    //     {
    //         break;
    //     }

    // }
    // start--; //由于第一组为99块，所以需要-1;
    // int i;
    // //给超级快直接管理的盘块赋值
    // for(i=0;i<s.s_nfree;i++)
    // {
    //     s.s_free[i] = start++;
    // }
    // captain c;

    // int not_in_table_num = FileSystem::DATA_ZONE_SIZE;
    // int group_num = 0;
    // while(1)
    // {
    //     if (not_in_table_num >= 100)
    //     {
    //         c.s_nfree = 100;
    //     }
    //     else
    //         c.s_nfree = not_in_table_num;

    //     not_in_table_num -= c.s_nfree;

    //     int i;
    //     for (i=0;i<c.s_nfree;i++)
    //     {
    //         if(i==0&&group_num==0)
    //         {
    //             c.s_free[i] = 0;
    //         }
    //         else
    //         {
    //             c.s_free[i] = FileSystem::DATA_ZONE_START_SECTOR + 100*group_num +i;
    //         }

    //     }
    //     memcpy(datablock+100*512*(group_num+1),&c,sizeof(c));
    //     if(not_in_table_num==0)
    //         break;

    // }
}

void init_datablock(char *datablock)
{
}


int main()
{
    int fd = open(disk_file_name, O_RDWR);
    int len = (FileSystem::DATA_ZONE_END_SECTOR + 1)*512;

    if (fd == -1)
    {
        cout << "img file isn't exist,create a new one " << endl;
        fd = open(disk_file_name, O_RDWR| O_CREAT, 0666);
        if (fd == -1)
        {
            cout << "create img file failed" << endl;
            exit(0);
        }
        for(int i=0;i<len;i++)
        {
            write(fd,"\0",1);
        }
        
        
        cout << "new img file create success"<<endl;
    }
    else{
        cout << "open img file success" <<endl;
    }
    
  
   
    /*把文件映射成虚拟内存地址*/
    void *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED,fd , 0);

    if ((void *)-1 == addr)
    {
        printf("map file error: %s\n", strerror(errno));
        exit(-1);
    }
    Kernel::Instance().Initialize( (char *)addr);
    init_blocks();



    //取消映射，关闭文件
    munmap(addr,len);
    close(fd);
}
