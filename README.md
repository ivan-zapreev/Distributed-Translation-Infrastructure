# Back Off Language Model(s) for SMT

<big>Author: Dr. Ivan S. Zapreev</big>: <https://nl.linkedin.com/in/zapreevis>

<big>Git-Hub</big>: <https://github.com/ivan-zapreev/Back-Off-Language-Model-SMT>

## Introduction
This is a fork project from the Automated-Translation-Tries project, made as a test exercise for automated machine translation (aiming at automated translation of text in different languages).

For machine translation it is important to estimate and compare the fluency of different possible translation outputs for the same source (i.e., foreign) sentence. This is commonly achieved by using
a language model, which measures the probability of a string (which is commonly a sentence). Since entire sentences are unlikely to occur more than once, this is often approximated by using sliding windows of words (n-grams) occurring in some training data.

### Background
An <i>n-gram</i> refers to a continuous sequence of n tokens. For instance, given the following sentence: our neighbor , who moved in recently , came by . If n = 3, then the possible n-grams of
this sentence include: 
<code>
"our neighbor ,"
"neighbor , who"
", who moved"
...
", came by"
"came by ."
</code>

Note that punctuation marks such as comma and full stop are treated just like any ‘real’ word and that all words are lowercased.

### References and Decisions
This project is originally based on two papers:
>        @inproceedings{DBLP:conf/acl/PaulsK11,
>        author    = {Adam Pauls and
>                       Dan Klein},
>          title     = {Faster and Smaller N-Gram Language Models},
>          booktitle = {The 49th Annual Meeting of the Association for Computational Linguistics:
>                       Human Language Technologies, Proceedings of the Conference, 19-24
>                       June, 2011, Portland, Oregon, {USA}},
>          pages     = {258--267},
>          year      = {2011},
>          crossref  = {DBLP:conf/acl/2011},
>          url       = {http://www.aclweb.org/anthology/P11-1027},
>          timestamp = {Fri, 02 Dec 2011 14:17:37 +0100},
>          biburl    = {http://dblp.uni-trier.de/rec/bib/conf/acl/PaulsK11},
>          bibsource = {dblp computer science bibliography, http://dblp.org}
>        }

and

>        @inproceedings{DBLP:conf/dateso/RobenekPS13,
>          author    = {Daniel Robenek and
>                       Jan Platos and
>                       V{\'{a}}clav Sn{\'{a}}sel},
>          title     = {Efficient In-memory Data Structures for n-grams Indexing},
>          booktitle = {Proceedings of the Dateso 2013 Annual International Workshop on DAtabases,
>                       TExts, Specifications and Objects, Pisek, Czech Republic, April 17,
>                       2013},
>          pages     = {48--58},
>          year      = {2013},
>          crossref  = {DBLP:conf/dateso/2013},
>          url       = {http://ceur-ws.org/Vol-971/paper21.pdf},
>          timestamp = {Mon, 22 Jul 2013 15:19:57 +0200},
>          biburl    = {http://dblp.uni-trier.de/rec/bib/conf/dateso/RobenekPS13},
>          bibsource = {dblp computer science bibliography, http://dblp.org}
>        }

The first paper discusses optimal Trie structures for storing the learned text corpus and the second indicates that using <i>std::unordered_map</i> of C++ delivers one of the best time and space performances, compared to other data structures, when using for Trie implementations

##License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

##Project structure
This is a Netbeans 8.0.2 project, and its' top-level structure is as follows:

>     ./docs/
>     ./inc/
>     ./src/
>     ./nbproject/
>     ./doxygen/
>     ./test-data/
>     ./LICENSE
>     ./Makefile
>     ./README.md
>     ./Doxyfile
>     ./make_centos.sh
>     ./make_debug_centos.sh
>     ./make_profile_centos.sh
>     ./make_release_centos.sh

Further, we give a few explanations of the structure above

* [Project-Folder]/
    * doc/ - contains the project-related documents
    * inc/ - stores the C++ header files used in the implementation
    * src/ - stores the C++ source files used in the implementation
    * nbproject/ - stores the Netbeans project data, such as makefiles
    * doxygen/ - stores the Doxygen-generated code documentation
    * test-data/ - stores the test-related data such as test models and query intput files, as well as the experimental results.
    * LICENSE - the code license (GPL 2.0)
    * Makefile - the Makefile used to build the project
    * README.md - this document
    * Doxyfile - the Doxygen configuration file
    * make_centos.sh - the script to build all available configurations on the Centos platform
    * make_debug_centos.sh - the script to build debug configuration on the Centos platform
    * make_profile_centos.sh - the script to profiling debug configuration on the Centos platform
    * make_release_centos.sh - the script to release debug configuration on the Centos platform

##Supported platforms
Currently this project supports two major platforms: Linux and Mac Os X. It has been successfully build and testd on:

* <big>Centos 6.6 64-bit</big> - Complete functionality.
* <big>Ubuntu 15.04 64-bit</big> - Complete functionality.
* <big>Mac OS X Yosemite 10.10 64-bit</big> - Limited by inability to collect memory-usage statistics.

Testing on 32-bit systems was not perfrormed.

##Building the project
Building this project requires gcc version 4.9.1 and higher. The project can be build in two ways:

+ From the Netbeans environment by running Build in the IDE
+ From the Linux command-line console
    - Open the console
    - Navigate to the project folder
    - Run <i>"make all"</i>
    - The binary will be generated and placed into <i>./dist/[type]_[platform]/</i> folder
    - The name of the executable is <i>back-off-language-model-smt</i>

In order to clean the project from the command line run <i>"make clean"</i>.

For the sake of simplicity and speed building on Centos platform is also possible by using make_*_centos.sh scripts. There are four of suck scripts available:

* make_debug_centos.sh - allows to build the debug configuration: ready to be used with valgrind and gdb.
* make_profile_centos.sh - allows to build the profiling configuration: ready to be used for gathering performance information for gprof: produce the gmon.out file.
* make_release_centos.sh - allows to build the release configuration: the version build for performance with platform specific optimiztions.
* make_centos.sh - allows to build all the above confgurations at once.

One can limit the debug-level printing of the code by changing the value of the <i>LOGER_MAX_LEVEL</i> constant in the <i>./inc/Configuration.hpp</i>. The possible range of values, with increasing logging level is: ERROR, WARNING, USAGE, RESULT, INFO, INFO1, INFO2, INFO3, DEBUG, DEBUG1, DEBUG2, DEBUG3, DEBUG4. It is also possible to vary the information level output by the program during its execution by specifying the command line flag, see the next section.


##Usage
In order to get the program usage information please run <i>./back-off-language-model-smt</i>
from the command line, the output of the program is supposed to be as follows:
        
        $ ../dist/Release__MacOs_/back-off-language-model-smt
        USAGE:	 ------------------------------------------------------------------ 
        USAGE:	|                 Back Off Language Model(s) for SMT     :)\___/(: |
        USAGE:	|                       Software version 1.0             {(@)v(@)} |
        USAGE:	|                         The Owl release.               {|~- -~|} |
        USAGE:	|             Copyright (C) Dr. Ivan S Zapreev, 2015     {/^'^'^\} |
        USAGE:	|  ═════════════════════════════════════════════════════════m-m══  |
        USAGE:	|        This software is distributed under GPL 2.0 license        |
        USAGE:	|          (GPL stands for GNU General Public License)             |
        USAGE:	|          The product comes with ABSOLUTELY NO WARRANTY.          |
        USAGE:	|   This is a free software, you are welcome to redistribute it.   |
        USAGE:	|                     Running in 64 bit mode!                      |
        USAGE:	|                 Build on: Sep 21 2015 17:26:44                   |
        USAGE:	 ------------------------------------------------------------------ 
        ERROR:	Incorrect number of arguments, expected >= 3, got 0
        USAGE:	Running: 
        USAGE:	  back-off-language-model-smt <model_file> <test_file> <trie_type> [debug-level]
        USAGE:	      <model_file> - a text file containing the back-off language model.
        USAGE:	                     This file is supposed to be in ARPA format, see: 
        USAGE:	                          http://www.speech.sri.com/projects/srilm/manpages/ngram-format.5.html
        USAGE:	                     for more details. We also allow doe tags listed here:
        USAGE:	                          https://msdn.microsoft.com/en-us/library/office/hh378460%28v=office.14%29.aspx
        USAGE:	      <test_file>  - a text file containing test data.
        USAGE:	                     The test file consists of a number of N-grams,
        USAGE:	                     where each line in the file consists of one N-gram.
        USAGE:	      <trie_type>  - the trie type, one of {c2dm, w2ch, c2wa, w2ca, c2dh, g2dm}
        USAGE:	     [debug-level] - the optional debug flag from { ERROR, WARN, USAGE, RESULT, INFO, INFO1, INFO2, INFO3, DEBUG, DEBUG1, DEBUG2, DEBUG3, DEBUG4 }
        USAGE:	Output: 
        USAGE:	    The program reads in the test queries from the <test_file>. 
        USAGE:	    Each of these lines is a N-grams of the following form, e.g: 
        USAGE:	       word1 word2 word3 word4 word5
        USAGE:	    For each of such N-grams the probability information is 
        USAGE:	    computed, based on the data from the <model_file>. For
        USAGE:	    example, for a N-gram such as:
        USAGE:	       mortgages had lured borrowers and
        USAGE:	    the program may give the following output:
        USAGE:	        log_10( Prob( word5 | word1 word2 word3 word4 ) ) = <log-probability>

##Implementation Details

In this section we mention a few implementation details, for more details see the source code documentation. At present the documentation is done in the Java-Doc style that is successfully accepted by Doxygen with the Doxygen option <i>JAVADOC_AUTOBRIEF</i> set to <i>YES</i>. The generated documentation is located in the <big>./doxygen/</big> folder of the project.

The code contains the following important source files:
* <big>ATries.hpp</big> - contains the common abstract class parent for all possible Trie classes
* <big>HashMapTrie.hpp/HashMapTrie.cpp</big> - contains the Hash-Map Trie implementation
* <big>Globals.hpp</big> - contains global configuration macros and some important globally used data types
* <big>Exceptions.hpp</big> - stores the implementations of the used exception classes
* <big>HashingUtils.hpp</big> - stores the hashing utility functions
* <big>NGramBuilder.hpp/NGramBuilder.cpp</big> - contains the class responsible for building n-grams from a line of text and storing it into Trie
* <big>TrieBuilder.hpp/TrieBuilder.cpp</big> - contains the class responsible for reading the text corpus and filling in the Trie using a NGramBuilder
* <big>StatisticsMonitor.hpp/StatisticsMonitor.cpp</big> - contains a class responsible for gathering memory and CPU usage statistics
* <big>BasicLogger.hpp/BasicLogger.cpp</big> - contains a basic logging facility class
* <big>main.cpp</big> - contains the entry point of the program and some utility functions including the one reading the test document and performing the queries on a filled in Trie instance.

##ToDo
* <big>HashMapTrie.hpp/HashMapTrie.cpp</big> - the current implementation of HashMapTrie is error prone to hash collisions. The build in checks are supposed to detect them and report an error message, but this is not enough. Yet the code was tested on data sets of up to 200.000 lines of text and no hash collisions were detected, so the hashing algorithms are currently sufficient good.
* <big>HashMapTrie.hpp/HashMapTrie.cpp</big> - the memory usage is not optimal, perhaps it is possible to provide a smarter implementation that will reduce the hash reference sizes so that less memory is used.
* <big>HashMapTrie.hpp/HashMapTrie.cpp</big> - When storing frequencies or generating word hashes and N-gram references one can easily get overflows resulting in wrong results, it would be nice to build in overflow checks into the code for safe executions.
* ~~<big>BasicLogger.hpp/BasicLogger.cpp</big> - by using compile time debug flags one can improve the applications performance by ensuring that the debug statements of higher order are compiled out. This is possible if the debugging is done using macros. Yet there will be no way to get finer debugging after the program is compiled.~~
* ~~<big>BasicLogger.hpp/BasicLogger.cpp</big> - currently uses <code>vsprintf(buffer, data, args);</code> for printing parameterized debug statements. Therefore, when printing arbitrary text from the corpus or test file one can get a segmentation fault if such a text contains un-escaped special control characters. Currently this problem is solved by not using <code>vsprintf(buffer, data, args);</code> for unsafe debug statements, but in general the implementation of the logging facility has to be robust enough to handle that.~~
* <big>Profiling</big> - although one takes case of not using dynamic allocated memory for the sake of performance improvements and avoiding memory leaks, it would be great to run <i>Valgrind</i> <http://valgrind.org/> and <i>Gprof</i> <https://sourceware.org/binutils/docs/gprof/> profiling to detect memory leaks, performance bottlenecks etc.
* <big>Testing</big> - the testing done with this code was limited. Potentially the Trie code, and the rest, still contains error. So it is recommended to add unit and functional tests for this project
* <big>Code</big> - in some places more of the old style C functions are used, which might have good equivalent in C++. Also, the naming convention is not always ideally followed. The using of Templates in the code might be to complex, although potentially gives some performance and genericity advantages.
 
##History
* <big>21.04.2015</big> - Created
* <big>27.07.2015</big> - Changed project name and some to-do's
