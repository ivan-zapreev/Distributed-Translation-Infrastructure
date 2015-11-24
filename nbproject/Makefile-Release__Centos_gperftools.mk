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
CND_CONF=Release__Centos_gperftools
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
	${OBJECTDIR}/src/C2DHybridTrie.o \
	${OBJECTDIR}/src/C2DMapTrie.o \
	${OBJECTDIR}/src/C2WArrayTrie.o \
	${OBJECTDIR}/src/G2DMapTrie.o \
	${OBJECTDIR}/src/H2DMapTrie.o \
	${OBJECTDIR}/src/Logger.o \
	${OBJECTDIR}/src/StatisticsMonitor.o \
	${OBJECTDIR}/src/W2CArrayTrie.o \
	${OBJECTDIR}/src/W2CHybridTrie.o \
	${OBJECTDIR}/src/main.o \
	${OBJECTDIR}/src/xxhash.o


# C Compiler Flags
CFLAGS=-march=native -Wall -Werror

# CC Compiler Flags
CCFLAGS=-mtune=native -std=c++0x -lrt -m64
CXXFLAGS=-mtune=native -std=c++0x -lrt -m64

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
	/opt/rh/devtoolset-3/root/usr/bin/g++ -o ${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt ${OBJECTFILES} ${LDLIBSOPTIONS} -mtune=native -lrt -m64 ${LDFLAGS} -lprofiler -ltcmalloc

${OBJECTDIR}/src/ARPAGramBuilder.o: nbproject/Makefile-${CND_CONF}.mk src/ARPAGramBuilder.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ARPAGramBuilder.o src/ARPAGramBuilder.cpp

${OBJECTDIR}/src/ARPATrieBuilder.o: nbproject/Makefile-${CND_CONF}.mk src/ARPATrieBuilder.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ARPATrieBuilder.o src/ARPATrieBuilder.cpp

${OBJECTDIR}/src/AWordIndex.o: nbproject/Makefile-${CND_CONF}.mk src/AWordIndex.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/AWordIndex.o src/AWordIndex.cpp

${OBJECTDIR}/src/ByteMGramId.o: nbproject/Makefile-${CND_CONF}.mk src/ByteMGramId.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ByteMGramId.o src/ByteMGramId.cpp

${OBJECTDIR}/src/C2DHybridTrie.o: nbproject/Makefile-${CND_CONF}.mk src/C2DHybridTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2DHybridTrie.o src/C2DHybridTrie.cpp

${OBJECTDIR}/src/C2DMapTrie.o: nbproject/Makefile-${CND_CONF}.mk src/C2DMapTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2DMapTrie.o src/C2DMapTrie.cpp

${OBJECTDIR}/src/C2WArrayTrie.o: nbproject/Makefile-${CND_CONF}.mk src/C2WArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2WArrayTrie.o src/C2WArrayTrie.cpp

${OBJECTDIR}/src/G2DMapTrie.o: nbproject/Makefile-${CND_CONF}.mk src/G2DMapTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/G2DMapTrie.o src/G2DMapTrie.cpp

${OBJECTDIR}/src/H2DMapTrie.o: nbproject/Makefile-${CND_CONF}.mk src/H2DMapTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Werror -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/H2DMapTrie.o src/H2DMapTrie.cpp

${OBJECTDIR}/src/Logger.o: nbproject/Makefile-${CND_CONF}.mk src/Logger.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Logger.o src/Logger.cpp

${OBJECTDIR}/src/StatisticsMonitor.o: nbproject/Makefile-${CND_CONF}.mk src/StatisticsMonitor.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/StatisticsMonitor.o src/StatisticsMonitor.cpp

${OBJECTDIR}/src/W2CArrayTrie.o: nbproject/Makefile-${CND_CONF}.mk src/W2CArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/W2CArrayTrie.o src/W2CArrayTrie.cpp

${OBJECTDIR}/src/W2CHybridTrie.o: nbproject/Makefile-${CND_CONF}.mk src/W2CHybridTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/W2CHybridTrie.o src/W2CHybridTrie.cpp

${OBJECTDIR}/src/main.o: nbproject/Makefile-${CND_CONF}.mk src/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -Iext -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

${OBJECTDIR}/src/xxhash.o: nbproject/Makefile-${CND_CONF}.mk src/xxhash.c 
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
