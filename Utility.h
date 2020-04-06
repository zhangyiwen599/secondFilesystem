#include <fstream>
#include <string>
#include <time.h>
using namespace std;
const string log_file_path = "log";


class Log
{
private:
    
    fstream  log_file;
   
public:
     enum LOGINFO{
        INFO,
        WARNING,
        ERROR
    };
    Log();
    ~Log();
    void write(LOGINFO info,string str);
    
};
extern Log log;
Log& getLog();

