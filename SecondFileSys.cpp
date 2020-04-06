
#include <string>

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


#include "Utility.h"
#include "Kernel.h"
#include "FileManager.h"
#include "OpenFileManager.h"

using namespace std;

int my_fcreate(char *filename,int mode)
{
	User &u = Kernel::Instance().GetUser();
    
    u.u_dirp = filename;
	u.u_arg[1] = Inode::IRWXU;
	u.u_error = User::MY_NOERROR;
	u.u_ar0 = 0;

	FileManager &fm = Kernel::Instance().GetFileManager();
    
	fm.Creat();    
	return u.u_ar0;
}

int my_fopen(char *pathname,int mode)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::MY_NOERROR;
	u.u_ar0 = 0;
	u.u_dirp = pathname;
	u.u_arg[1] = mode;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Open();
	return u.u_ar0;
}

int my_fread(int fd,char *des,int len)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::MY_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;
	u.u_arg[1] = long(des);
	u.u_arg[2] = len;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Read();
	return u.u_ar0;
}

int my_fwrite(int fd,char *src,int len)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::MY_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;
	u.u_arg[1] = long(src);
	u.u_arg[2] = len;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Write();
	return u.u_ar0;
}




// void fdelete(char* name)
// {
// 	User &u = Kernel::Instance().GetUser();
// 	u.u_error = User::MY_NOERROR;
// 	u.u_ar0 = 0;
// 	u.u_dirp = name;
// 	FileManager &fm = Kernel::Instance().GetFileManager();
// 	fm.UnLink();
// }

int my_flseek(int fd,int position,int ptrname)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::MY_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;
	u.u_arg[1] = position;
	u.u_arg[2] = ptrname;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Seek();
	return u.u_ar0;
}

void my_fclose(int fd)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error =User::MY_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;

    FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Close();
}


void my_ls()
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::MY_NOERROR;
    cout <<1 <<endl;
	int fd = my_fopen(u.u_curdir, (File::FREAD) );
     cout <<2<<endl;
	char directory_data[32] = { 0 };
	while(true)
	{
		if (my_fread(fd, directory_data, 32) == 0) {
            cout <<4 <<endl;
			return;
		}
		else
		{
            cout <<3 <<endl;
			DirectoryEntry *t = (DirectoryEntry*)directory_data;
			if (t->m_ino == 0)
				continue;
			Inode * pInode = g_InodeTable.IGet(t->m_ino);
			if(pInode->i_mode & pInode->IFDIR)
				cout << "[" << t->m_name << "]" << endl;
			else
				cout << t->m_name << endl;
			memset(directory_data, 0, 32);
		}
	}
	my_fclose(fd);
}



void my_mkdir(char *dirname)
{
	int defaultmode = 040755; 
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::MY_NOERROR;
	u.u_dirp = dirname;
	u.u_arg[1] = defaultmode;
	u.u_arg[2] = 0;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.MkNod();

}

void my_cd(char *dirname)
{
    User &u = Kernel::Instance().GetUser();
	u.u_error = User::MY_NOERROR;
	u.u_dirp = dirname;
	u.u_arg[0] = long(dirname);
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.ChDir();
}

void my_goBack(){
	char temp_curdir[128] = { 0 };
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::MY_NOERROR;
	char *last=strrchr(u.u_curdir, '/');
	if (last == u.u_curdir&&u.u_curdir[1]==0)
	{
		return;
	}
	else if (last == u.u_curdir)
	{
		temp_curdir[0] = '/';
	}
	else {
		int i = 0;
		for (char *pc = u.u_curdir; pc != last; pc++)
		{
			temp_curdir[i] = u.u_curdir[i];
			i++;
		}
	}
	u.u_dirp = temp_curdir;
	u.u_arg[0] = long(temp_curdir);
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.ChDir();
}

void exitOS(char *addr,int len)
{
	BufferManager &bm = Kernel::Instance().GetBufferManager();
	bm.Bflush();
	// msync(addr, len, MS_SYNC);
	InodeTable *mit = Kernel::Instance().GetFileManager().m_InodeTable;
	mit->UpdateInodeTable();
}

void init_sys()
{
    cout << "start system initialize" << endl;
	FileManager& fm = Kernel::Instance().GetFileManager();
	fm.rootDirInode = g_InodeTable.IGet(FileSystem::ROOTINO);
	fm.rootDirInode->i_flag &= (~Inode::ILOCK);

	Kernel::Instance().GetFileSystem().LoadSuperBlock();
	User& u =Kernel::Instance().GetUser();
	u.u_cdir = g_InodeTable.IGet(FileSystem::ROOTINO);
	strcpy(u.u_curdir, "/");
	cout << "system initialize ok" << endl;
}




int main()
{
    const char *disk_file_name = "c.img";
    int fd = open(disk_file_name, O_RDWR);
    int len = (FileSystem::DATA_ZONE_END_SECTOR + 1)*512;
    if (fd == -1)
    {
        cout << "img file isn't exist,create a new one " << endl;
    }
    else
    {
        cout << "open img file success"<<endl;
    }
    

    void *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED,fd , 0);
    if ((void *)-1 == addr)
    {
        printf("map file error: %s\n", strerror(errno));
        exit(-1);
    }

    Kernel::Instance().Initialize( (char *)addr);
    init_sys();

    // while(1)
    // {
    //     string cmd;
    //     std::getline(cin,cmd);


    // }

    int res = my_fcreate("myfirstfile",0);
    char *a=NULL;

    cout << sizeof(a) <<endl;

    res = my_fopen("myfirstfile",0);

    cout << res <<endl;
    my_fclose(res);
    cout << res <<endl;
    my_mkdir("test");
    cout << res <<endl;
    my_ls();
    
    cout << res <<endl;
    
    munmap(addr,len);
    close(fd);
}