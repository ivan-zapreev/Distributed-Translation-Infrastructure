/* 
 * File:   StatisticsMonitor.cpp
 * Author: zapreevis
 * 
 * Created on April 18, 2015, 12:18 PM
 */

#include "StatisticsMonitor.hpp"
#include "Exceptions.hpp"
#include "BasicLogger.hpp"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sstream>

/**
 * This implementation is derived from 
 * http://locklessinc.com/articles/memory_usage/
 * This here is actually C-style code and also pretty ugly.
 * 
 * ToDo: Refactor this code into C++ 
 */
void StatisticsMonitor::getMemoryStatistics( TMemotyUsage & memStat ) throw (Exception) {
	char *line;
	char *vmsize = NULL;
	char *vmpeak = NULL;
	char *vmrss = NULL;
	char *vmhwm = NULL;
	size_t len;
	FILE *f;

	line = (char*) malloc(128);
	len = 128;
	
	f = fopen("/proc/self/status", "r");
	if ( !f ) throw Exception("Unable to open /proc/self/status for reading!");
	
	/* Read memory size data from /proc/self/status */
	while (!vmsize || !vmpeak || !vmrss || !vmhwm)
	{
		if (getline(&line, &len, f) == -1)
		{
			/* Some of the information isn't there, die */
			throw Exception("Unable to read memoty statistics data from /proc/self/status");
		}
		
		/* Find VmPeak */
		if (!strncmp(line, "VmPeak:", 7))
		{
			vmpeak = strdup(&line[7]);
		}
		
		/* Find VmSize */
		else if (!strncmp(line, "VmSize:", 7))
		{
			vmsize = strdup(&line[7]);
		}
		
		/* Find VmRSS */
		else if (!strncmp(line, "VmRSS:", 6))
		{
			vmrss = strdup(&line[7]);
		}
		
		/* Find VmHWM */
		else if (!strncmp(line, "VmHWM:", 6))
		{
			vmhwm = strdup(&line[7]);
		}
	}
	free(line);
	
	fclose(f);

	/* Get rid of " kB\n"*/
	len = strlen(vmsize);
	vmsize[len - 4] = 0;
	len = strlen(vmpeak);
	vmpeak[len - 4] = 0;
	len = strlen(vmrss);
	vmrss[len - 4] = 0;
	len = strlen(vmhwm);
	vmhwm[len - 4] = 0;

	/* Parse the results */
        memStat.vmpeak = atoi(vmpeak);
        memStat.vmsize = atoi(vmsize);
        memStat.vmrss = atoi(vmrss);
        memStat.vmhwm = atoi(vmhwm);

        /* Print some info and debug information */
         BasicLogger::printDebug("read: vmsize=%s Kb, vmpeak=%s Kb, vmrss=%s Kb, vmhwm=%s Kb", vmsize, vmpeak, vmrss, vmhwm);
         BasicLogger::printDebug("parsed: vmsize=%d Kb, vmpeak=%d Kb, vmrss=%d Kb, vmhwm=%d Kb",
                                  memStat.vmsize, memStat.vmpeak, memStat.vmrss, memStat.vmhwm);
	
        /* Free the allocated memory */
	free(vmpeak);
	free(vmsize);
	free(vmrss);
	free(vmhwm);
}