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
CCC=g++ -std=c++0x -m64
CXX=g++ -std=c++0x -m64
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-MacOSX
CND_DLIB_EXT=dylib
CND_CONF=Release__MacOs_
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/ALayeredTrie.o \
	${OBJECTDIR}/src/ARPAGramBuilder.o \
	${OBJECTDIR}/src/ARPATrieBuilder.o \
	${OBJECTDIR}/src/AWordIndex.o \
	${OBJECTDIR}/src/C2DHashMapTrie.o \
	${OBJECTDIR}/src/C2DMapArrayTrie.o \
	${OBJECTDIR}/src/C2WOrderedArrayTrie.o \
	${OBJECTDIR}/src/G2DHashMapTrie.o \
	${OBJECTDIR}/src/Logger.o \
	${OBJECTDIR}/src/MGrams.o \
	${OBJECTDIR}/src/StatisticsMonitor.o \
	${OBJECTDIR}/src/W2CHybridMemoryTrie.o \
	${OBJECTDIR}/src/W2COrderedArrayTrie.o \
	${OBJECTDIR}/src/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-march=native -Wall -Werror
CXXFLAGS=-march=native -Wall -Werror

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
	g++ -o ${CND_DISTDIR}/${CND_CONF}/back-off-language-model-smt ${OBJECTFILES} ${LDLIBSOPTIONS} -march=native -m64 -Wall -Werror

${OBJECTDIR}/src/ALayeredTrie.o: nbproject/Makefile-${CND_CONF}.mk src/ALayeredTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ALayeredTrie.o src/ALayeredTrie.cpp

${OBJECTDIR}/src/ARPAGramBuilder.o: nbproject/Makefile-${CND_CONF}.mk src/ARPAGramBuilder.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ARPAGramBuilder.o src/ARPAGramBuilder.cpp

${OBJECTDIR}/src/ARPATrieBuilder.o: nbproject/Makefile-${CND_CONF}.mk src/ARPATrieBuilder.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/ARPATrieBuilder.o src/ARPATrieBuilder.cpp

${OBJECTDIR}/src/AWordIndex.o: nbproject/Makefile-${CND_CONF}.mk src/AWordIndex.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/AWordIndex.o src/AWordIndex.cpp

${OBJECTDIR}/src/C2DHashMapTrie.o: nbproject/Makefile-${CND_CONF}.mk src/C2DHashMapTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2DHashMapTrie.o src/C2DHashMapTrie.cpp

${OBJECTDIR}/src/C2DMapArrayTrie.o: nbproject/Makefile-${CND_CONF}.mk src/C2DMapArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2DMapArrayTrie.o src/C2DMapArrayTrie.cpp

${OBJECTDIR}/src/C2WOrderedArrayTrie.o: nbproject/Makefile-${CND_CONF}.mk src/C2WOrderedArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/C2WOrderedArrayTrie.o src/C2WOrderedArrayTrie.cpp

${OBJECTDIR}/src/G2DHashMapTrie.o: nbproject/Makefile-${CND_CONF}.mk src/G2DHashMapTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/G2DHashMapTrie.o src/G2DHashMapTrie.cpp

${OBJECTDIR}/src/Logger.o: nbproject/Makefile-${CND_CONF}.mk src/Logger.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Logger.o src/Logger.cpp

${OBJECTDIR}/src/MGrams.o: nbproject/Makefile-${CND_CONF}.mk src/MGrams.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/MGrams.o src/MGrams.cpp

${OBJECTDIR}/src/StatisticsMonitor.o: nbproject/Makefile-${CND_CONF}.mk src/StatisticsMonitor.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/StatisticsMonitor.o src/StatisticsMonitor.cpp

${OBJECTDIR}/src/W2CHybridMemoryTrie.o: nbproject/Makefile-${CND_CONF}.mk src/W2CHybridMemoryTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/W2CHybridMemoryTrie.o src/W2CHybridMemoryTrie.cpp

${OBJECTDIR}/src/W2COrderedArrayTrie.o: nbproject/Makefile-${CND_CONF}.mk src/W2COrderedArrayTrie.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/W2COrderedArrayTrie.o src/W2COrderedArrayTrie.cpp

${OBJECTDIR}/src/main.o: nbproject/Makefile-${CND_CONF}.mk src/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Werror -Iinc -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

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
