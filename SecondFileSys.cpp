


#include "SystemCall.h"
#include <sstream>
#include <vector>


int main()
{
    const char *disk_file_name = "c.img";
    int fd = open(disk_file_name, O_RDWR);
    
    if (fd == -1)
    {
        cout << "img file isn't exist,create a new one " << endl;
    }
    else
    {
        cout << "open img file success"<<endl;
    }
    struct stat file_st;
	fstat(fd,&file_st);
  	int len = file_st.st_size;

    void *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED,fd , 0);
    if ((void *)-1 == addr)
    {
        printf("map file error: %s\n", strerror(errno));
        exit(-1);
    }

    Kernel::Instance().Initialize( (char *)addr);
    init_sys();

	/*
		支持的命令：
		1. [ls] 展示当前
	*/
    while(1)
    {
        string str;
		cout << "root@secondfilesys:# ";
        std::getline(cin,str);
		//spilt space
		if(str.length()==0)
			continue;
		istringstream in(str);
    	vector<string> cmd;

		string t;
		char temp[100];
		char buffer[1024];
		while (in >> t) {
			cmd.push_back(t);
		}
		if(str == "help" || str =="h")
		{
			cout << "\tvalid commands" <<endl;
			cout << "\t1. [ls] list all file and dir in current path\t"<<endl;
			cout << "\t2. [cd] change directory \t"<<endl;
			cout << "\t3. [mkdir] create a new dir\t"<<endl;
			cout << "\t4. [mkfile] create a new file\t"<<endl;
			cout << "\t5. [read] read data from file\t"<<endl;
			cout << "\t6. [write] write data to file\t"<<endl;
			cout << "\t7. [pTOs] copy file in pc to secondfilesys\t"<<endl;
			cout << "\t8. [sTOp] copy file in secondfilesys to pc\t"<<endl;
			cout << "\t9. [exit] exit system\t"<<endl;
			continue;
		}
		else if(cmd[0]=="ls")
		{
			my_ls();
		}
		else if(cmd[0]=="cd")
		{
			if(cmd.size()!=2)
			{
				cout << "invalid arg num, you should write like: cd [dirname]" <<endl;
				continue;
			}
			if(cmd[1]=="..")
			{
				my_goBack();
			}
			else
			{
				
				strcpy(temp,cmd[1].c_str());
				my_cd(temp);
			}
		}
		else if(cmd[0]=="mkdir")
		{
			if(cmd.size()!=2)
			{
				cout << "invalid arg num, you should write like: mkdir [dirname]" <<endl;
				continue;
			}
			strcpy(temp,cmd[1].c_str());
			my_mkdir(temp);
		}
		else if(cmd[0]=="mkfile")
		{
			if(cmd.size()!=2)
			{
				cout << "invalid arg num, you should write like: mkfile [filename]" <<endl;
				continue;
			}
			strcpy(temp,cmd[1].c_str());
			my_fcreate(temp,0);
		}
		else if(cmd[0]=="read")
		{
			if(cmd.size()!=2)
			{
				cout << "invalid arg num, you should write like: read [filename]" <<endl;
				continue;
			}
			strcpy(temp,cmd[1].c_str());
			int t_fd = my_fopen(temp,File::FREAD);
			if(t_fd == -1)
			{
				cout << "file not exist" <<endl;
				continue;
			}
			int rd_len = my_fread(t_fd,buffer,sizeof(buffer));
			cout << "read: "<<rd_len<<" bytes from file\n data: ";
			cout << buffer <<endl;
		}
		else if(cmd[0]=="write")
		{
			if(cmd.size()!=3)
			{
				cout << "invalid arg num, you should write like: write [filename] [data]" <<endl;
				continue;
			}
			strcpy(temp,cmd[1].c_str());
			strcpy(buffer,cmd[2].c_str());
			int t_fd = my_fopen(temp,File::FWRITE);
			if(t_fd == -1)
			{
				cout << "file not exist" <<endl;
				continue;
			}
			int wr_len = my_fwrite(t_fd,buffer,strlen(buffer));
			cout << "write: "<<wr_len<<" bytes to file" <<endl;
		}
		else if(cmd[0]=="pTOs")
		{
			if(cmd.size()!=3)
			{
				cout << "invalid arg num, you should write like: [pTOs] [ori_filename] [dest_filename]" <<endl;
				continue;
			}
			strcpy(temp,cmd[1].c_str());
			strcpy(buffer,cmd[2].c_str());
			int o_fd = open(temp,O_RDWR);
			if(o_fd == -1)
			{
				cout << "ori file not exist"<<endl;
				continue;
			}
			int d_fd = my_fcreate(buffer,0);
			if(d_fd==-1)
			{
				cout << "dest file create failed" <<endl;
				close(o_fd);
				continue;
			}
			int read_len = read(o_fd,buffer,sizeof(buffer));
			buffer[read_len] = '\0';
			int wr_len = my_fwrite(d_fd,buffer,strlen(buffer));
			cout << "copy: "<<wr_len<<" bytes from "<<cmd[1] <<" to "<< cmd[2] <<endl;
		}
		else if(cmd[0]=="sTOp")
		{
			if(cmd.size()!=3)
			{
				cout << "invalid arg num, you should write like: [sTOp] [ori_filename] [dest_filename]" <<endl;
				continue;
			}
			strcpy(temp,cmd[1].c_str());
			strcpy(buffer,cmd[2].c_str());
			int o_fd = my_fopen(temp,File::FREAD);
			if(o_fd == -1)
			{
				cout << "ori file not exist"<<endl;
				continue;
			}
			int d_fd = open(buffer,O_CREAT | O_RDWR);
			if(d_fd==-1)
			{
				cout << "dest file create failed" <<endl;
				my_fclose(o_fd);
				continue;
			}

			int read_len = my_fread(o_fd,buffer,sizeof(buffer));
			buffer[read_len ] = '\0';
			int wr_len = write(d_fd,buffer,strlen(buffer));
			cout << "copy: "<<wr_len<<" bytes from "<<cmd[1] <<" to "<< cmd[2] <<endl;
		}
		else if(cmd[0]=="exit")
		{
			break;
		}
		else{
			cout << "invalid command, input [help] or [h] to see all commands"<<endl;
		}

    }

    
    exitOS((char *)addr,len);
    close(fd);
}