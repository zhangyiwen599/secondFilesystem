#include "Kernel.h"
#include <iostream>
using namespace std;

Kernel Kernel::instance;


/*
 * 高速缓存管理全局manager
 */
BufferManager g_BufferManager;


/*
 * 文件系统相关全局manager
 */
FileSystem g_FileSystem;
FileManager g_FileManager;

User g_User;
/* 系统全局超级块SuperBlock对象 */
SuperBlock g_spb;

Kernel::Kernel()
{
}

Kernel::~Kernel()
{
}

Kernel& Kernel::Instance()
{
	return Kernel::instance;
}





void Kernel::InitBuffer(char * memdisk)
{
	this->m_BufferManager = &g_BufferManager;
	// this->m_DeviceManager = &g_DeviceManager;

	cout<<"Initialize Buffer..."<<endl;
	this->GetBufferManager().Initialize(memdisk);
	cout<<"OK."<<endl;
}

void Kernel::InitFileSystem()
{
	this->m_FileSystem = &g_FileSystem;
	this->m_FileManager = &g_FileManager;
	this->m_User = &g_User;

	cout<<"Initialize File System..."<<endl;
	this->GetFileSystem().Initialize();
	cout<<"OK"<<endl;

	cout<<"Initialize File Manager..."<<endl;
	this->GetFileManager().Initialize();
	cout<<"OK"<<endl;
}

void Kernel::Initialize(char * memdisk)
{
	
	InitBuffer(memdisk);
	InitFileSystem();
}





BufferManager& Kernel::GetBufferManager()
{
	return *(this->m_BufferManager);
}


FileSystem& Kernel::GetFileSystem()
{
	return *(this->m_FileSystem);
}

FileManager& Kernel::GetFileManager()
{
	return *(this->m_FileManager);
}

User& Kernel::GetUser()
{
	return *(this->m_User);
}
