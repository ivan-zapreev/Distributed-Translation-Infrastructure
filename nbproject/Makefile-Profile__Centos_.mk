#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=/opt/rh/devtoolset-3/root/usr/bin/g++
CXX=/opt/rh/devtoolset-3/root/usr/bin/g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-MacOSX
CND_DLIB_EXT=dylib
CND_CONF=Profile__Centos_
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/ARPAGramBuilder.o \
	${OBJECTDIR}/src/ARPATrieBuilder.o \
	${OBJECTDIR}/src/AWordIndex.o \
	${OBJECTDIR}/src/ByteMGramId.o \
	${OBJECTDIR}/src/C2DHashMapTrie.o \
	${OBJECTDIR}/src/C2DMapArrayTrie.o \
	${OBJECTDIR}/src/C2WOrderedArrayTrie.o \
	${OBJECTDIR}/src/G2DHashMapTrie.o \
	${OBJECTDIR}/src/LayeredTrieDriver.o \
	${OBJECTDIR}/src/Logger.o \
	${OBJECTDIR}/src/StatisticsMonitor.o \
	${OBJECTDIR}/src/W2CHybridMemoryTrie.o \
	${OBJECTDIR}/src/W2COrderedArrayTrie.o \
	${OBJECTDIR}/src/main.o \
	${OBJECTDIR}/src/xxhash.o


# C Compiler Flags
CFLAGS=-march=native -Wall -Werror

# CC Compiler Flags
CCFLAGS=-std=c++0x -lrt -m64 -pg -O3
CXXFLAGS=-std=c++0x -lrt -m64 -pg -O3

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt

${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}
	/opt/rh/devtoolset-3/root/usr/bin/g++ -o ${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt ${OBJECTFILES} ${LDLIBSOPTIONS} -std=c++0x -lrt -m64 -pg -O3

${OBJECTDIR}/src/ARPAGramBuilder.o: src/ARPAGramBuilder.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ARPAGramBuilder.o src/ARPAGramBuilder.cpp

${OBJECTDIR}/src/ARPATrieBuilder.o: src/ARPATrieBuilder.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ARPATrieBuilder.o src/ARPATrieBuilder.cpp

${OBJECTDIR}/src/AWordIndex.o: src/AWordIndex.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/AWordIndex.o src/AWordIndex.cpp

${OBJECTDIR}/src/ByteMGramId.o: src/ByteMGramId.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ByteMGramId.o src/ByteMGramId.cpp

${OBJECTDIR}/src/C2DHashMapTrie.o: src/C2DHashMapTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2DHashMapTrie.o src/C2DHashMapTrie.cpp

${OBJECTDIR}/src/C2DMapArrayTrie.o: src/C2DMapArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2DMapArrayTrie.o src/C2DMapArrayTrie.cpp

${OBJECTDIR}/src/C2WOrderedArrayTrie.o: src/C2WOrderedArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2WOrderedArrayTrie.o src/C2WOrderedArrayTrie.cpp

${OBJECTDIR}/src/G2DHashMapTrie.o: src/G2DHashMapTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/G2DHashMapTrie.o src/G2DHashMapTrie.cpp

${OBJECTDIR}/src/LayeredTrieDriver.o: src/LayeredTrieDriver.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/LayeredTrieDriver.o src/LayeredTrieDriver.cpp

${OBJECTDIR}/src/Logger.o: src/Logger.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Logger.o src/Logger.cpp

${OBJECTDIR}/src/StatisticsMonitor.o: src/StatisticsMonitor.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/StatisticsMonitor.o src/StatisticsMonitor.cpp

${OBJECTDIR}/src/W2CHybridMemoryTrie.o: src/W2CHybridMemoryTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/W2CHybridMemoryTrie.o src/W2CHybridMemoryTrie.cpp

${OBJECTDIR}/src/W2COrderedArrayTrie.o: src/W2COrderedArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/W2COrderedArrayTrie.o src/W2COrderedArrayTrie.cpp

${OBJECTDIR}/src/main.o: src/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

${OBJECTDIR}/src/xxhash.o: src/xxhash.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/xxhash.o src/xxhash.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
