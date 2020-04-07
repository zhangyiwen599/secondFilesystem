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
    // ��һ����99�飬���涼��100�� ʣ���ɳ�����ֱ�ӹ���
    //s.s_nfree = (FileSystem::DATA_ZONE_SIZE -99)%100;
    FileSystem filesys = Kernel::Instance().GetFileSystem();


    /* 
	 * ��������( 128 <= blkno < 1023 )��ÿ��������Free(blkno)һ�£�
	 * ���ɽ�����free block����"ջ��ջ"��ʽ��֯������
	 */
    // �˴������Unixv6++��д��
    g_spb.s_nfree = 0;
    for (int blkno = FileSystem::DATA_ZONE_END_SECTOR; blkno >= FileSystem::DATA_ZONE_START_SECTOR; --blkno)
    {
        filesys.Free(blkno);
    }
    //ѭ����Ϻ� s_nfree �Լ� s_free[] �Ͷ�����˳�ʼ��
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
	g_spb.s_inode[g_spb.s_ninode++] = 0;    //��ջ���� 0# DiskInode
    //cout << 4 << endl;

    /****************** ��0# DiskInode��������Ŀ¼��DiskInode ******************/
    Inode* pNode = filesys.IAlloc();

    //cout << 5 << endl;

    User& u = Kernel::Instance().GetUser();
    pNode->i_flag |= (Inode::IACC | Inode::IUPD);
	pNode->i_mode = Inode::IALLOC | Inode::IFDIR /* Most vital!! */| Inode::IREAD | Inode::IWRITE | Inode::IEXEC | (Inode::IREAD >> 3) | (Inode::IWRITE >> 3) | (Inode::IEXEC >> 3) | (Inode::IREAD >> 6) | (Inode::IWRITE >> 6) | (Inode::IEXEC >> 6);
    pNode->i_nlink  = 1;
    

    g_InodeTable.IPut(pNode);	/* ��rootDir DiskInodeд����� */
    /****************** ��0# DiskInode��������Ŀ¼��DiskInode END ******************/
     
    g_spb.s_ronly = 0;
    g_spb.s_fmod = 1;

    //��super block������Ϣд�����
    filesys.Update();


    cout << "Initialize Block success" <<endl;
}


void spb_init(SuperBlock &sb)
{
	sb.s_isize = FileSystem::INODE_ZONE_SIZE;
	sb.s_fsize = FileSystem::DATA_ZONE_END_SECTOR+1;

	//��һ��99�� ��������һ�ٿ�һ�� ʣ�µı�������ֱ�ӹ���
	sb.s_nfree = (FileSystem::DATA_ZONE_SIZE - 99) % 100;

	//������ֱ�ӹ���Ŀ����̿�ĵ�һ���̿���̿��
	//��������
	int start_last_datablk = FileSystem::DATA_ZONE_START_SECTOR;
	for (;;)
		if ((start_last_datablk + 100 - 1) < FileSystem::DATA_ZONE_END_SECTOR)//�ж�ʣ���̿��Ƿ���100��
			start_last_datablk += 100;
		else
			break;
	start_last_datablk--;
	for (int i = 0; i < sb.s_nfree; i++)
		sb.s_free[i] = start_last_datablk + i;

	sb.s_ninode = 100;
	for (int i = 0; i < sb.s_ninode; i++)
		sb.s_inode[i] = i ;//ע������ֻ��diskinode�ı�ţ�����ȡ�õ�ʱ��Ҫ�����̿��ת��

	sb.s_fmod = 0;
	sb.s_ronly = 0;
	
	
}

void init_datablock(char *data)
{
	struct {
		int nfree;//������еĸ���
		int free[100];//������е�������
	}tmp_table;

	int last_datablk_num = FileSystem::DATA_ZONE_SIZE;//δ�����������̿������
	//ע:�������ӷ�,����ĳ�ʼ��������
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

	//����rootDiskInode�ĳ�ʼֵ
	di[0].d_mode = Inode::IFDIR;
	di[0].d_mode |= Inode::IEXEC;
	//di[0].d_nlink = 888;

	char *datablock = new char[FileSystem::DATA_ZONE_SIZE * 512];
	memset(datablock, 0, FileSystem::DATA_ZONE_SIZE * 512);
	init_datablock(datablock);

	write(fd, &spb,  sizeof(SuperBlock));
	write(fd, di, FileSystem::INODE_ZONE_SIZE*FileSystem::INODE_NUMBER_PER_SECTOR * sizeof(DiskInode));
	write(fd, datablock, FileSystem::DATA_ZONE_SIZE * 512);

	// cout << "��ʽ���������" << endl;
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
   
    /*���ļ�ӳ��������ڴ��ַ*/
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

    //ȡ��ӳ�䣬�ر��ļ�
   	exitOS((char *)addr,len);
    close(fd);
}
