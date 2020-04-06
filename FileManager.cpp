#include "FileManager.h"
#include "Kernel.h"
#include "Utility.h"


/*==========================class FileManager===============================*/
FileManager::FileManager()
{
	//nothing to do here
}

FileManager::~FileManager()
{
	//nothing to do here
}

void FileManager::Initialize()
{
	this->m_FileSystem = &Kernel::Instance().GetFileSystem();

	this->m_InodeTable = &g_InodeTable;
	this->m_OpenFileTable = &g_OpenFileTable;
	this->m_spb = &g_spb;
	this->m_InodeTable->Initialize();
	
}

/*
 * ���ܣ����ļ�
 * Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������i_count ++��
 * */
void FileManager::Open()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	pInode = this->NameI(NextChar, FileManager::OPEN);	/* 0 = Open, not create */
	/* û���ҵ���Ӧ��Inode */
	if ( NULL == pInode )
	{
		getLog().write(Log::WARNING,string("can't find Inode"));
		return;
	}
	this->Open1(pInode, u.u_arg[1], 0);
	getLog().write(Log::INFO,string("function Open() return succ"));
}

/*
 * ���ܣ�����һ���µ��ļ�
 * Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������Ӧ���� 1��
 * */
void FileManager::Creat()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	unsigned int newACCMode = u.u_arg[1] & (Inode::IRWXU|Inode::IRWXG|Inode::IRWXO);

	/* ����Ŀ¼��ģʽΪ1����ʾ����������Ŀ¼����д���������� */
	pInode = this->NameI(NextChar, FileManager::CREATE);
	/* û���ҵ���Ӧ��Inode����NameI���� */

	if ( NULL == pInode )
	{
		if(u.u_error)
		{
			getLog().write(Log::WARNING,string("NameI error"));
			return;
		}
			
		/* ����Inode */
		pInode = this->MakNode( newACCMode & (~Inode::ISVTX) );
		/* ����ʧ�� */
		if ( NULL == pInode )
		{
			getLog().write(Log::WARNING,string("Create Inode failed"));
			return;
		}

		/* 
		 * �����ϣ�������ֲ����ڣ�ʹ�ò���trf = 2������open1()��
		 * ����Ҫ����Ȩ�޼�飬��Ϊ�ոս������ļ���Ȩ�޺ʹ������mode
		 * ����ʾ��Ȩ��������һ���ġ�
		 */
		this->Open1(pInode, File::FWRITE, 2);
	}
	else
	{
		/* ���NameI()�������Ѿ�����Ҫ�������ļ�������ո��ļ������㷨ITrunc()����UIDû�иı�
		 * ԭ��UNIX��������������ļ�����ȥ�����½����ļ�һ����Ȼ�������ļ������ߺ�����Ȩ��ʽû�䡣
		 * Ҳ����˵creatָ����RWX������Ч��
		 * ������Ϊ���ǲ������ģ�Ӧ�øı䡣
		 * ���ڵ�ʵ�֣�creatָ����RWX������Ч */
		this->Open1(pInode, File::FWRITE, 1);
		pInode->i_mode |= newACCMode;
	}
	getLog().write(Log::INFO,string("function Create() return succ"));
}

/* 
* trf == 0��open����
* trf == 1��creat���ã�creat�ļ���ʱ��������ͬ�ļ������ļ�
* trf == 2��creat���ã�creat�ļ���ʱ��δ������ͬ�ļ������ļ��������ļ�����ʱ��һ������
* mode���������ļ�ģʽ����ʾ�ļ������� ����д���Ƕ�д
*/
void FileManager::Open1(Inode* pInode, int mode, int trf)
{
	User& u = Kernel::Instance().GetUser();

	/* 
	 * ����ϣ�����ļ��Ѵ��ڵ�����£���trf == 0��trf == 1����Ȩ�޼��
	 * �����ϣ�������ֲ����ڣ���trf == 2������Ҫ����Ȩ�޼�飬��Ϊ�ս���
	 * ���ļ���Ȩ�޺ʹ���Ĳ���mode������ʾ��Ȩ��������һ���ġ�
	 */
	if (trf != 2)
	{
		if ( mode & File::FREAD )
		{
			/* ����Ȩ�� */
			this->Access(pInode, Inode::IREAD);
		}
		if ( mode & File::FWRITE )
		{
			/* ���дȨ�� */
			this->Access(pInode, Inode::IWRITE);
			/* ϵͳ����ȥдĿ¼�ļ��ǲ������� */
			if ( (pInode->i_mode & Inode::IFMT) == Inode::IFDIR )
			{
				u.u_error = User::MY_EISDIR;
				getLog().write(Log::WARNING,string("write Index file not allowed"));
			}
		}
	}

	if ( u.u_error )
	{
		this->m_InodeTable->IPut(pInode);
		return;
	}

	/* ��creat�ļ���ʱ��������ͬ�ļ������ļ����ͷŸ��ļ���ռ�ݵ������̿� */
	if ( 1 == trf )
	{
		pInode->ITrunc();
	}

	/* ����inode! 
	 * ����Ŀ¼�����漰�����Ĵ��̶�д�������ڼ���̻���˯��
	 * ��ˣ����̱������������漰��i�ڵ㡣�����NameI��ִ�е�IGet����������
	 * �����ˣ����������п��ܻ���������л��Ĳ��������Խ���i�ڵ㡣
	 */
	// pInode->Prele();

	/* ������ļ����ƿ�File�ṹ */
	File* pFile = this->m_OpenFileTable->FAlloc();
	if ( NULL == pFile )
	{
		getLog().write(Log::WARNING,string("alloc new file struct failed, free Inode"));
		this->m_InodeTable->IPut(pInode);
		return;
	}
	/* ���ô��ļ���ʽ������File�ṹ���ڴ�Inode�Ĺ�����ϵ */
	pFile->f_flag = mode & (File::FREAD | File::FWRITE);
	pFile->f_inode = pInode;

	/* �����豸�򿪺��� */
	// pInode->OpenI(mode & File::FWRITE);

	/* Ϊ�򿪻��ߴ����ļ��ĸ�����Դ���ѳɹ����䣬�������� */
	// if ( u.u_error == 0 )
	// {
	// 	return;
	// }
	// else	/* ����������ͷ���Դ */
	// {
	// 	/* �ͷŴ��ļ������� */
	// 	int fd = u.u_ar0[User::EAX];
	// 	if(fd != -1)
	// 	{
	// 		u.u_ofiles.SetF(fd, NULL);
	// 		/* �ݼ�File�ṹ��Inode�����ü��� ,File�ṹû���� f_countΪ0�����ͷ�File�ṹ��*/
	// 		pFile->f_count--;
	// 	}
	// 	this->m_InodeTable->IPut(pInode);
	// }
}

void FileManager::Close()
{
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	/* ��ȡ���ļ����ƿ�File�ṹ */
	File* pFile = u.u_ofiles.GetF(fd);
	if ( NULL == pFile )
	{
		getLog().write(Log::WARNING,string("in funtion Close(), can't find File struct"));
		return;
	}

	/* �ͷŴ��ļ�������fd���ݼ�File�ṹ���ü��� */
	u.u_ofiles.SetF(fd, NULL);
	this->m_OpenFileTable->CloseF(pFile);
	getLog().write(Log::INFO,string("function Close() return succ"));
}

void FileManager::Seek()
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	pFile = u.u_ofiles.GetF(fd);
	if ( NULL == pFile )
	{
		getLog().write(Log::WARNING,string("In function Seek(),can't find File struct"));
		return;  /* ��FILE�����ڣ�GetF��������� */
	}

	// /* �ܵ��ļ�������seek */
	// if ( pFile->f_flag & File::FPIPE )
	// {
	// 	u.u_error = User::ESPIPE;
	// 	return;
	// }

	int offset = u.u_arg[1];

	/* ���u.u_arg[2]��3 ~ 5֮�䣬��ô���ȵ�λ���ֽڱ�Ϊ512�ֽ� */
	if ( u.u_arg[2] > 2 )
	{
		offset = offset << 9;
		u.u_arg[2] -= 3;
	}

	switch ( u.u_arg[2] )
	{
		/* ��дλ������Ϊoffset */
		case 0:
			pFile->f_offset = offset;
			break;
		/* ��дλ�ü�offset(�����ɸ�) */
		case 1:
			pFile->f_offset += offset;
			break;
		/* ��дλ�õ���Ϊ�ļ����ȼ�offset */
		case 2:
			pFile->f_offset = pFile->f_inode->i_size + offset;
			break;
	}
	getLog().write(Log::INFO,string("function Seek() return succ"));
}


void FileManager::FStat()
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	pFile = u.u_ofiles.GetF(fd);
	if ( NULL == pFile )
	{
		getLog().write(Log::WARNING,string("in function FStat() failed to find File Struct"));
		return;
	}

	/* u.u_arg[1] = pStatBuf */
	this->Stat1(pFile->f_inode, u.u_arg[1]);
	getLog().write(Log::INFO,string("function FStat() return succ"));

}

void FileManager::Stat()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
	if ( NULL == pInode )
	{
		getLog().write(Log::WARNING,string("in function Stat() failed to find File Struct"));
		return;
	}
	this->Stat1(pInode, u.u_arg[1]);
	this->m_InodeTable->IPut(pInode);
	getLog().write(Log::INFO,string("function Stat() return succ"));

}

void FileManager::Stat1(Inode* pInode, unsigned long statBuf)
{
	Buf* pBuf;
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	pInode->IUpdate();
	pBuf = bufMgr.Bread(FileSystem::INODE_ZONE_START_SECTOR + pInode->i_number / FileSystem::INODE_NUMBER_PER_SECTOR );

	/* ��pָ�򻺴����б��Ϊinumber���Inode��ƫ��λ�� */
	unsigned char* p = pBuf->b_addr + (pInode->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
	// Utility::DWordCopy( (int *)p, (int *)statBuf, sizeof(DiskInode)/sizeof(int) );
	memcpy(&statBuf,p,sizeof(DiskInode));

	bufMgr.Brelse(pBuf);
}

void FileManager::Read()
{
	/* ֱ�ӵ���Rdwr()�������� */
	this->Rdwr(File::FREAD);
}

void FileManager::Write()
{
	/* ֱ�ӵ���Rdwr()�������� */
	this->Rdwr(File::FWRITE);
}

void FileManager::Rdwr( enum File::FileFlags mode )
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();

	/* ����Read()/Write()��ϵͳ���ò���fd��ȡ���ļ����ƿ�ṹ */
	pFile = u.u_ofiles.GetF(u.u_arg[0]);	/* fd */
	if ( NULL == pFile )
	{
		/* �����ڸô��ļ���GetF�Ѿ����ù������룬�������ﲻ��Ҫ�������� */
		u.u_error = User::MY_EBADF;
		getLog().write(Log::WARNING,string("in function Rdwr() failed to find File Struct"));
		return;
	}


	/* ��д��ģʽ����ȷ */
	if ( (pFile->f_flag & mode) == 0 )
	{
		getLog().write(Log::WARNING,string("In function rdwr, rdwr mode error"));
		return;
	}

	u.u_IOParam.m_Base = (unsigned char *)u.u_arg[1];	/* Ŀ�껺������ַ */
	u.u_IOParam.m_Count = u.u_arg[2];		/* Ҫ���/д���ֽ��� */
	// u.u_segflg = 0;		/* User Space I/O�����������Ҫ�����ݶλ��û�ջ�� */

	/* �ܵ���д */
	
	/* ��ͨ�ļ���д �����д�����ļ������ļ�ʵʩ������ʣ���������ȣ�ÿ��ϵͳ���á�
	Ϊ��Inode����Ҫ��������������NFlock()��NFrele()��
	�ⲻ��V6����ơ�read��writeϵͳ���ö��ڴ�i�ڵ�������Ϊ�˸�ʵʩIO�Ľ����ṩһ�µ��ļ���ͼ��*/
	
	
	/* �����ļ���ʼ��λ�� */
	u.u_IOParam.m_Offset = pFile->f_offset;
	if ( File::FREAD == mode )
	{
		pFile->f_inode->ReadI();
	}
	else
	{
		pFile->f_inode->WriteI();
	}

	/* ���ݶ�д�������ƶ��ļ���дƫ��ָ�� */
	pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);
	

	/* ����ʵ�ʶ�д���ֽ������޸Ĵ��ϵͳ���÷���ֵ�ĺ���ջ��Ԫ */
	u.u_ar0 = u.u_arg[2] - u.u_IOParam.m_Count;
	getLog().write(Log::INFO,string("function Rdwr() return succ"));

}



/* ����NULL��ʾĿ¼����ʧ�ܣ������Ǹ�ָ�룬ָ���ļ����ڴ��i�ڵ� ���������ڴ�i�ڵ�  */
Inode* FileManager::NameI( char (*func)(), enum DirectorySearchMode mode )
{
	Inode* pInode;
	Buf* pBuf;
	char curchar;
	char* pChar;
	int freeEntryOffset;	/* �Դ����ļ�ģʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ���� */

	User& u = Kernel::Instance().GetUser();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	/* 
	 * �����·����'/'��ͷ�ģ��Ӹ�Ŀ¼��ʼ������
	 * ����ӽ��̵�ǰ����Ŀ¼��ʼ������
	 */
	pInode = u.u_cdir;
	if ( '/' == (curchar = (*func)()) )
	{
		pInode = this->rootDirInode;
	}

	
	/* ����Inode�Ƿ����ڱ�ʹ�ã��Լ���֤������Ŀ¼���������и�Inode�����ͷ� */
	this->m_InodeTable->IGet( pInode->i_number);

	/* ��������////a//b ����·�� ����·���ȼ���/a/b */
	while ( '/' == curchar )
	{
		curchar = (*func)();
	}
	/* �����ͼ���ĺ�ɾ����ǰĿ¼�ļ������ */
	if ( '\0' == curchar && mode != FileManager::OPEN )
	{
		u.u_error = User::MY_ENOENT;
		getLog().write(Log::WARNING,string("In function NameI, Change Index file error"));

		goto out;
	}

	/* ���ѭ��ÿ�δ���pathname��һ��·������ */
	while (true)
	{
		/* ����������ͷŵ�ǰ��������Ŀ¼�ļ�Inode�����˳� */
		if ( u.u_error != User::MY_NOERROR )
		{
			break;	/* goto out; */
		}

		/* ����·��������ϣ�������ӦInodeָ�롣Ŀ¼�����ɹ����ء� */
		if ( '\0' == curchar )
		{
			return pInode;
		}

		/* ���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����Inode��Դ���˳� */
		if ( (pInode->i_mode & Inode::IFMT) != Inode::IFDIR )
		{
			u.u_error = User::MY_ENOTDIR;
			getLog().write(Log::WARNING,string("In function NameI(), Not index file, file mode:")+to_string(pInode->i_mode));

			break;	/* goto out; */
		}

		/* ����Ŀ¼����Ȩ�޼��,IEXEC��Ŀ¼�ļ��б�ʾ����Ȩ�� */
		if ( this->Access(pInode, Inode::IEXEC) )
		{
			u.u_error = User::MY_EACCES;
			getLog().write(Log::WARNING,string("In function NameI(),No Authority of Index file "));

			break;	/* ���߱�Ŀ¼����Ȩ�ޣ�goto out; */
		}

		/* 
		 * ��Pathname�е�ǰ׼������ƥ���·������������u.u_dbuf[]�У�
		 * ���ں�Ŀ¼����бȽϡ�
		 */
		pChar = &(u.u_dbuf[0]);
		while ( '/' != curchar && '\0' != curchar)
		{
			if ( pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]) )
			{
				*pChar = curchar;
				pChar++;
			}
			curchar = (*func)();
		}
		/* ��u_dbufʣ��Ĳ������Ϊ'\0' */
		while ( pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]) )
		{
			*pChar = '\0';
			pChar++;
		}

		/* ��������////a//b ����·�� ����·���ȼ���/a/b */
		while ( '/' == curchar )
		{
			curchar = (*func)();
		}

		if ( u.u_error != User::MY_NOERROR )
		{
			break; /* goto out; */
		}

		/* �ڲ�ѭ�����ֶ���u.u_dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
		u.u_IOParam.m_Offset = 0;
		/* ����ΪĿ¼����� �����հ׵�Ŀ¼��*/
		u.u_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
		freeEntryOffset = 0;
		pBuf = NULL;

		while (true)
		{
			/* ��Ŀ¼���Ѿ�������� */
			if ( 0 == u.u_IOParam.m_Count )
			{
				if ( NULL != pBuf )
				{
					bufMgr.Brelse(pBuf);
				}
				/* ����Ǵ������ļ� */
				if ( FileManager::CREATE == mode && curchar == '\0' )
				{
					/* �жϸ�Ŀ¼�Ƿ��д */
					if ( this->Access(pInode, Inode::IWRITE) )
					{
						u.u_error = User::MY_EACCES;
						getLog().write(Log::WARNING,string("In function NameI(),can't Write Index file"));
						
						goto out;	/* Failed */
					}

					/* ����Ŀ¼Inodeָ�뱣���������Ժ�дĿ¼��WriteDir()�������õ� */
					u.u_pdir = pInode;

					if ( freeEntryOffset )	/* �˱�������˿���Ŀ¼��λ��Ŀ¼�ļ��е�ƫ���� */
					{
						/* ������Ŀ¼��ƫ��������u���У�дĿ¼��WriteDir()���õ� */
						u.u_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
					}
					else  /*���⣺Ϊ��if��֧û����IUPD��־��  ������Ϊ�ļ��ĳ���û�б�ѽ*/
					{
						pInode->i_flag |= Inode::IUPD;
					}
					/* �ҵ�����д��Ŀ���Ŀ¼��λ�ã�NameI()�������� */
					getLog().write(Log::INFO,string("function NameI() return suc,find free index, return null"));
					return NULL;
				}
				
				/* Ŀ¼��������϶�û���ҵ�ƥ����ͷ����Inode��Դ�����Ƴ� */
				u.u_error = User::MY_ENOENT;
				goto out;
			}

			/* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
			if ( 0 == u.u_IOParam.m_Offset % Inode::BLOCK_SIZE )
			{
				if ( NULL != pBuf )
				{
					bufMgr.Brelse(pBuf);
				}
				/* ����Ҫ���������̿�� */
				int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE );
				pBuf = bufMgr.Bread(phyBlkno );
			}

			/* û�ж��굱ǰĿ¼���̿飬���ȡ��һĿ¼����u.u_dent */
			int* src = (int *)(pBuf->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE));
			// Utility::DWordCopy( src, (int *)&u.u_dent, sizeof(DirectoryEntry)/sizeof(int) );
			memcpy(&u.u_dent,src,sizeof(DirectoryEntry));

			u.u_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
			u.u_IOParam.m_Count--;

			/* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
			if ( 0 == u.u_dent.m_ino )
			{
				if ( 0 == freeEntryOffset )
				{
					freeEntryOffset = u.u_IOParam.m_Offset;
				}
				/* ��������Ŀ¼������Ƚ���һĿ¼�� */
				continue;
			}

			int i;
			for ( i = 0; i < DirectoryEntry::DIRSIZ; i++ )
			{
				if ( u.u_dbuf[i] != u.u_dent.m_name[i] )
				{
					break;	/* ƥ����ĳһ�ַ�����������forѭ�� */
				}
			}

			if( i < DirectoryEntry::DIRSIZ )
			{
				/* ����Ҫ������Ŀ¼�����ƥ����һĿ¼�� */
				continue;
			}
			else
			{
				/* Ŀ¼��ƥ��ɹ����ص����While(true)ѭ�� */
				break;
			}
		}

		/* 
		 * ���ڲ�Ŀ¼��ƥ��ѭ�������˴���˵��pathname��
		 * ��ǰ·������ƥ��ɹ��ˣ�����ƥ��pathname����һ·��
		 * ������ֱ������'\0'������
		 */
		if ( NULL != pBuf )
		{
			bufMgr.Brelse(pBuf);
		}

		/* �����ɾ���������򷵻ظ�Ŀ¼Inode����Ҫɾ���ļ���Inode����u.u_dent.m_ino�� */
		if ( FileManager::DELETE == mode && '\0' == curchar )
		{
			/* ����Ը�Ŀ¼û��д��Ȩ�� */
			if ( this->Access(pInode, Inode::IWRITE) )
			{
				u.u_error = User::MY_EACCES;
				getLog().write(Log::WARNING,string("In function NameI(),No Authority write parent Index file "));
				break;	/* goto out; */
			}
			getLog().write(Log::INFO,string("function NameI() return suc"));

			return pInode;
		}

		/* 
		 * ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼Inode������ƥ��ɹ���
		 * Ŀ¼��m_ino�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���Inode��
		 */
		short dev = pInode->i_dev;
		this->m_InodeTable->IPut(pInode);
		pInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
		/* �ص����While(true)ѭ��������ƥ��Pathname����һ·������ */

		if ( NULL == pInode )	/* ��ȡʧ�� */
		{
			getLog().write(Log::WARNING,string("function NameI() return succ, return NULL, can't find Inode"));
			return NULL;
		}
	}
out:
	this->m_InodeTable->IPut(pInode);
	return NULL;
}

char FileManager::NextChar()
{
	User& u = Kernel::Instance().GetUser();
	
	/* u.u_dirpָ��pathname�е��ַ� */
	return *u.u_dirp++;
}

/* ��creat���á�
 * Ϊ�´������ļ�д�µ�i�ڵ���µ�Ŀ¼��
 * ���ص�pInode�����������ڴ�i�ڵ㣬���е�i_count�� 1��
 *
 * �ڳ����������� WriteDir��������������Լ���Ŀ¼��д����Ŀ¼���޸ĸ�Ŀ¼�ļ���i�ڵ� ������д�ش��̡�
 *
 */
Inode* FileManager::MakNode( unsigned int mode )
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	/* ����һ������DiskInode������������ȫ����� */
	pInode = this->m_FileSystem->IAlloc();
	if( NULL ==	pInode )
	{
		getLog().write(Log::WARNING,string("function MakeNode() , No free Inode"));
		
		return NULL;
	}

	pInode->i_flag |= (Inode::IACC | Inode::IUPD);
	pInode->i_mode = mode | Inode::IALLOC;
	pInode->i_nlink = 1;
	// pInode->i_uid = u.u_uid;
	// pInode->i_gid = u.u_gid;
	/* ��Ŀ¼��д��u.u_dent�����д��Ŀ¼�ļ� */
	this->WriteDir(pInode);
	return pInode;
}

void FileManager::WriteDir( Inode* pInode )
{
	User& u = Kernel::Instance().GetUser();

	/* ����Ŀ¼����Inode��Ų��� */
	u.u_dent.m_ino = pInode->i_number;

	/* ����Ŀ¼����pathname�������� */
	for ( int i = 0; i < DirectoryEntry::DIRSIZ; i++ )
	{
		u.u_dent.m_name[i] = u.u_dbuf[i];
	}

	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;
	// u.u_segflg = 1;

	/* ��Ŀ¼��д�븸Ŀ¼�ļ� */
	u.u_pdir->WriteI();
	this->m_InodeTable->IPut(u.u_pdir);
}

void FileManager::SetCurDir(char* pathname)
{
	User& u = Kernel::Instance().GetUser();
	
	/* ·�����ǴӸ�Ŀ¼'/'��ʼ����������u.u_curdir������ϵ�ǰ·������ */
	if ( pathname[0] != '/' )
	{
		int length = strlen(u.u_curdir);
		if ( u.u_curdir[length - 1] != '/' )
		{
			u.u_curdir[length] = '/';
			length++;
		}
		// Utility::StringCopy(pathname, u.u_curdir + length);
		strcpy( u.u_curdir + length,pathname);
	}
	else	/* ����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼ */
	{
		// Utility::StringCopy(pathname, u.u_curdir);
		strcpy( u.u_curdir,pathname);
	}
}

/*
 * ����ֵ��0����ʾӵ�д��ļ���Ȩ�ޣ�1��ʾû������ķ���Ȩ�ޡ��ļ�δ�ܴ򿪵�ԭ���¼��u.u_error�����С�
 */
int FileManager::Access( Inode* pInode, unsigned int mode )
{
	User& u = Kernel::Instance().GetUser();

	/* ����д��Ȩ�ޣ���������ļ�ϵͳ�Ƿ���ֻ���� */
	if ( Inode::IWRITE == mode )
	{
		if( this->m_FileSystem->GetFS()->s_ronly != 0 )
		{
			u.u_error = User::MY_EROFS;
			return 1;
		}
	}
	/* 
	 * ���ڳ����û�����д�κ��ļ�����������
	 * ��Ҫִ��ĳ�ļ�ʱ��������i_mode�п�ִ�б�־
	 */
	if ( 1)
	{
		if ( Inode::IEXEC == mode && ( pInode->i_mode & (Inode::IEXEC | (Inode::IEXEC >> 3) | (Inode::IEXEC >> 6)) ) == 0 )
		{
			u.u_error = User::MY_EACCES;
		    getLog().write(Log::WARNING,string("function Access() , unwriteable"));
			
			return 1;
		}
		return 0;	/* Permission Check Succeed! */
	}
	// if ( u.u_uid != pInode->i_uid )
	// {
	// 	mode = mode >> 3;
	// 	if ( u.u_gid != pInode->i_gid )
	// 	{
	// 		mode = mode >> 3;
	// 	}
	// }
	// if ( (pInode->i_mode & mode) != 0 )
	// {
	// 	return 0;
	// }

	u.u_error = User::MY_EACCES;
	return 1;
}



void FileManager::ChDir()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
	if ( NULL == pInode )
	{
		getLog().write(Log::WARNING,string("function ChDir() ,no such file"));
		return;
	}
	/* ���������ļ�����Ŀ¼�ļ� */
	if ( (pInode->i_mode & Inode::IFMT) != Inode::IFDIR )
	{
		u.u_error = User::MY_ENOTDIR;
		getLog().write(Log::WARNING,string("function Chdir() ,not index file"));

		this->m_InodeTable->IPut(pInode);
		return;
	}
	if ( this->Access(pInode, Inode::IEXEC) )
	{
		this->m_InodeTable->IPut(pInode);
		getLog().write(Log::INFO,string("function Chdir() , Permission denied"));

		return;
	}
	this->m_InodeTable->IPut(u.u_cdir);
	u.u_cdir = pInode;
	// pInode->Prele();

	this->SetCurDir((char *)u.u_arg[0] /* pathname */);
	getLog().write(Log::INFO,string("function Chdir() , return succ"));

}



void FileManager::UnLink()
{
	Inode* pInode;
	Inode* pDeleteInode;
	User& u = Kernel::Instance().GetUser();

	pDeleteInode = this->NameI(FileManager::NextChar, FileManager::DELETE);
	if ( NULL == pDeleteInode )
	{
		getLog().write(Log::WARNING,string("function Unlink() , can't find unlink file"));
		return;
	}
	// pDeleteInode->Prele();

	pInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
	if ( NULL == pInode )
	{
		// Utility::Panic("unlink -- iget");
		getLog().write(Log::WARNING,string("function Unlink(), get Inode failed"));

	}
	/* ֻ��root����unlinkĿ¼�ļ� */
	if ( (pInode->i_mode & Inode::IFMT) == Inode::IFDIR)
	{
		this->m_InodeTable->IPut(pDeleteInode);
		this->m_InodeTable->IPut(pInode);
		return;
	}
	/* д��������Ŀ¼�� */
	u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;
	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	
	u.u_dent.m_ino = 0;
	pDeleteInode->WriteI();

	/* �޸�inode�� */
	pInode->i_nlink--;
	pInode->i_flag |= Inode::IUPD;

	this->m_InodeTable->IPut(pDeleteInode);
	this->m_InodeTable->IPut(pInode);
	getLog().write(Log::INFO,string("function Unlink() , return succ"));

}

void FileManager::MkNod()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	/* ���uid�Ƿ���root����ϵͳ����ֻ��uid==rootʱ�ſɱ����� */
	if (1)
	{
		pInode = this->NameI(FileManager::NextChar, FileManager::CREATE);
		/* Ҫ�������ļ��Ѿ�����,���ﲢ����ȥ���Ǵ��ļ� */
		if ( pInode != NULL )
		{
			u.u_error = User::MY_EEXIST;
			getLog().write(Log::WARNING,string("function MkNod() , file exist"));
			this->m_InodeTable->IPut(pInode);
			return;
		}
	}
	else
	{
		/* ��root�û�ִ��mknod()ϵͳ���÷���User::EPERM */
		u.u_error = User::MY_EPERM;
		return;
	}
	/* û��ͨ��SUser()�ļ�� */
	if ( User::MY_NOERROR != u.u_error )
	{
		return;	/* û����Ҫ�ͷŵ���Դ��ֱ���˳� */
	}
	pInode = this->MakNode(u.u_arg[1]);
	if ( NULL == pInode )
	{
		return;
	}
	/* ���������豸�ļ� */
	if ( (pInode->i_mode & (Inode::IFBLK | Inode::IFCHR)) != 0 )
	{
		pInode->i_addr[0] = u.u_arg[2];
	}
	this->m_InodeTable->IPut(pInode);
}
/*==========================class DirectoryEntry===============================*/
DirectoryEntry::DirectoryEntry()
{
	this->m_ino = 0;
	this->m_name[0] = '\0';
}

DirectoryEntry::~DirectoryEntry()
{
	//nothing to do here
}
