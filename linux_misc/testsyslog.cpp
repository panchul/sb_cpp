//
// syslog playground
// writes to /var/log/syslog
//

#include <stdio.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
        openlog("mytestlog", LOG_PERROR, LOG_USER);
        int i=0;

        while(++i < 10)
        {
                syslog(LOG_INFO, "something %d", i);
        }
       	closelog();
        return 0;
}
