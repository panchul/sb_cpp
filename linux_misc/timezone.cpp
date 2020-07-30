#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> // for isprint

#include <time.h>
// #include <tzfile.h>

main()
{
	struct tm *t;
	time_t current_time;

	tzset();
	timezone = 10 * 60 * 60;

	current_time = time(NULL);
	//	current_time -= 4*60*60;
	t = localtime(&current_time);

	printf(" current_time %ld\n", current_time);
	printf(" timezone %d\n", timezone);
	printf(" tzname \"%s\"\n", tzname);
	printf(" daylight %d\n", daylight);
	//	printf(" tzfile \"%s\"\n", tzfile);

	//	setenv("TZ", "EST5EDT", 1);
	printf(" getenv(\"TZ\") \"%s\"\n", getenv("TZ"));
	printf(" current time is %4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d\n",
		   t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		   t->tm_hour, t->tm_min, t->tm_sec);

	printf("sizeof(time_t) = %d\n", sizeof(time_t));
	//--------
	char mytz[256];
	printf("\n\n\nset tz to new value");
	sprintf(mytz, "EDT 4:0");
	setenv("TZ", mytz, 1);
	tzset();
	current_time = time(NULL);
	t = localtime(&current_time);
	printf(" getenv(\"TZ\") \"%s\"\n", getenv("TZ"));
	printf(" current time is %4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d\n",
		   t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		   t->tm_hour, t->tm_min, t->tm_sec);
}
