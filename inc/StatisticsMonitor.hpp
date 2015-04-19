/* 
 * File:   StatisticsMonitor.hpp
 * Author: zapreevis
 *
 * Created on April 18, 2015, 12:18 PM
 */

#ifndef STATISTICSMONITOR_HPP
#define	STATISTICSMONITOR_HPP

#include "Exceptions.hpp"

/**
 * This structure stores the memory statistics.
 * Resident Set Size: number of pages the process has
 * in real memory.  This is just the pages which count
 * toward text, data, or stack space.  This does not
 * include pages which have not been demand-loaded in,
 * or which are swapped out.
 * For more information see http://man7.org/linux/man-pages/man5/proc.5.html
 */
typedef struct {
    //Virtual memory size in Kb
    unsigned int vmsize;
    //Peak virtual memory size in Kb
    unsigned int vmpeak;
    //Resident set size in Kb
    unsigned int vmrss;
    //Peak resident set size in Kb
    unsigned int vmhwm;
} TMemotyUsage;

/**
 * This class is responsible for monitoring the program statistics, such as the used memory and CPU times.
 * This class is a trivial singleton
 */
class StatisticsMonitor {
public:
    
    /**
     * Allows to get the current memory usage of the process.
     * @param memStat this is an out parameter that will store the obtained data
     * @throws Exception in case the memory statistics can not be obtained.
     */
    static void getMemoryStatistics(TMemotyUsage & memStat) throw (Exception);
    
    /**
     * This function returns the current CPU time as given in the article
     * http://nadeausoftware.com/articles/2012/03/c_c_tip_how_measure_cpu_time_benchmarking
     * @return Returns the amount of CPU time used by the current process,
     *         in seconds, or -1.0 if an error occurred.
     */
    static double getCPUTime();
    
private:
    StatisticsMonitor(){}
    StatisticsMonitor(const StatisticsMonitor& orig){}
    virtual ~StatisticsMonitor(){}

};

#endif	/* STATISTICSMONITOR_HPP */

