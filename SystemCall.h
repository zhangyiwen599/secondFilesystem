#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string>

#include <iostream>

#include "Utility.h"
#include "Kernel.h"
#include "FileManager.h"
#include "OpenFileManager.h"
using namespace std;

int my_fcreate(char *filename,int mode);
int my_fopen(char *pathname,int mode);
int my_fread(int fd,char *des,int len);
int my_fwrite(int fd,char *src,int len);
int my_flseek(int fd,int position,int ptrname);
void my_fclose(int fd);
void my_ls();
void my_mkdir(char *dirname);
void my_cd(char *dirname);
void my_goBack();
void exitOS(char *addr,int len);
void init_sys();
void fdelete(char* name);









