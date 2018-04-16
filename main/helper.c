#include "helper.h"

uint64_t millis(){


	struct timeval tv;
	gettimeofday(&tv, NULL);

	unsigned long long millisecondsSinceEpoch =
    (unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000;



	// time_t rawtime;
	// struct tm *now;
	// char time_string[80];

   	// time( &rawtime );
	// now = localtime(&rawtime );
	// now
	// // strftime(time_string, 80, "%c", now);
	// // ESP_LOGI(LOGT, "Starting up at %s", time_string);
	// return now;
	return (uint64_t)millisecondsSinceEpoch;
}