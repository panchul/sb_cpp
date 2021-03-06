
// Neat discussion at
// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
// with some performance metrics
//

//Results for total time to run the 100,000 calls averaged over 5 runs,
//
// Method exists_test0 (ifstream): **0.485s**
// Method exists_test1 (FILE fopen): **0.302s**
// Method exists_test2 (posix access()): **0.202s**
// Method exists_test3 (posix stat()): **0.134s**

#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <fstream>

inline bool exists_test0(const std::string &name)
{
    ifstream f(name.c_str());
    return f.good();
}

inline bool exists_test1(const std::string &name)
{
    if (FILE *file = fopen(name.c_str(), "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
}

inline bool exists_test2(const std::string &name)
{
    return (access(name.c_str(), F_OK) != -1);
}

inline bool exists_test3(const std::string &name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}
