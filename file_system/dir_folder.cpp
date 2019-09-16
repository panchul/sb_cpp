//
// playing around with options from the discussion at
// https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
//
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>

static void list_dir(const char *path)
{
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n",entry->d_name);
    }

    closedir(dir);
}

int main()
{
    list_dir("./");
/*    char name[]="./";
    len = strlen(name);
    dirp = opendir(".");
    while ((dp = readdir(dirp)) != NULL)
        std::cout << db->d_name << "\n";
        //if (dp->d_namlen == len && !strcmp(dp->d_name, name)) {
        //        (void)closedir(dirp);
        //        return FOUND;
        //}
    (void)closedir(dirp);
    return NOT_FOUND;
*/    
    return 0;
}

/*
// Specify that it's for C++17 like so:
//
// $ g++ -std=c++1z -lc++fs -lc++experimental dir_folder.cpp
//

#include <string>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main()
{
    std::string path = "/."; // path to dir
    for (const auto & entry : fs::directory_iterator(path))
        std::cout << entry.path() << std::endl;
}
*/
