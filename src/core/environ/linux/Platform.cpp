#include "Platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/resource.h>

void TVPGetMemoryInfo(TVPMemoryInfo &m)
{
    /* to read /proc/meminfo */
    FILE* meminfo;
    char buffer[100] = {0};
    char* end;
    int found = 0;

    /* Try to read /proc/meminfo, bail out if fails */
	meminfo = fopen("/proc/meminfo", "r");

    static const char
        pszMemFree[] = "MemFree:",
        pszMemTotal[] = "MemTotal:",
        pszSwapTotal[] = "SwapTotal:",
        pszSwapFree[] = "SwapFree:",
        pszVmallocTotal[] = "VmallocTotal:",
        pszVmallocUsed[] = "VmallocUsed:";

    /* Read each line untill we got all we ned */
    while( fgets( buffer, sizeof( buffer ), meminfo ) )
    {
        if( strstr( buffer, pszMemFree ) == buffer )
        {
            m.MemFree = strtol( buffer + sizeof(pszMemFree), &end, 10 );
            found++;
        }
        else if( strstr( buffer, pszMemTotal ) == buffer )
        {
            m.MemTotal = strtol( buffer + sizeof(pszMemTotal), &end, 10 );
            found++;
        }
        else if( strstr( buffer, pszSwapTotal ) == buffer )
        {
            m.SwapTotal = strtol( buffer + sizeof(pszSwapTotal), &end, 10 );
            found++;
            break;
        }
        else if( strstr( buffer, pszSwapFree ) == buffer )
        {
            m.SwapFree = strtol( buffer + sizeof(pszSwapFree), &end, 10 );
            found++;
            break;
        }
        else if( strstr( buffer, pszVmallocTotal ) == buffer )
        {
            m.VirtualTotal = strtol( buffer + sizeof(pszVmallocTotal), &end, 10 );
            found++;
            break;
        }
        else if( strstr( buffer, pszVmallocUsed ) == buffer )
        {
            m.VirtualUsed = strtol( buffer + sizeof(pszVmallocUsed), &end, 10 );
            found++;
            break;
        }
    }
    fclose(meminfo);
}

#include <sched.h>
void TVPRelinquishCPU(){
	sched_yield();
}

void TVP_utime(const char *name, time_t modtime) {
	timeval mt[2];
	mt[0].tv_sec = modtime;
	mt[0].tv_usec = 0;
	mt[1].tv_sec = modtime;
	mt[1].tv_usec = 0;
	utimes(name, mt);
}
