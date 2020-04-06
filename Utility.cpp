#include "Utility.h"

Log log;

Log::Log()
{
    log_file.open(log_file_path);
}

Log::~Log()
{
    log_file.close();
}


void Log::write(Log::LOGINFO info,string  str)
{
    time_t cur = time(0);
    std::string curtime(ctime(&cur));
    curtime.pop_back();
    curtime += " ";
    log_file << curtime;
    if(info==Log::ERROR)
    {
        log_file<<"[ERROR]";
    }
    else if(info==Log::INFO)
    {
        log_file<<"[INFO]";
        
    }
    else if(info==Log::WARNING)
    {
        log_file<<"[WARNING]";
    }
    else{
        log_file<<"[UNDEFINED]";
    }

    log_file<<str<<endl;

}

Log& getLog()
{
    return log;
}
