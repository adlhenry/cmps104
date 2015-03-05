# Author: Adam Henry, adlhenry@ucsc.edu
# $Id: Makefile,v 1.1 2015-03-01 16:26:03-08 - - $

MKFILE    = Makefile
DEPSFILE  = ${MKFILE}.deps
NOINCLUDE = ci clean spotless
NEEDINCL  = ${filter ${NOINCLUDE}, ${MAKECMDGOALS}}
GMAKE     = gmake --no-print-directory

GCC       = g++ -g -O0 -Wall -Wextra -std=gnu++0x
MKDEPS    = g++ -MM -std=gnu++0x

CSOURCE   = main.cpp auxlib.cpp stringset.cpp
CHEADER   = auxlib.h stringset.h
OBJECTS   = ${CSOURCE:.cpp=.o}
EXECBIN   = oc
SUBMITS   = ${CHEADER} ${CSOURCE} ${MKFILE} README
SOURCES   = ${SUBMITS}

# Define for current project
PROJECT   = cmps104a-wm.s15 asg1

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${GCC} -o $@ ${OBJECTS}

%.o : %.cpp
	${GCC} -c $<

ci : ${SOURCES}
	cid + ${SOURCES}
	checksource ${SUBMITS}

clean :
	- rm ${OBJECTS} ${DEPSFILE} ${EXECBIN}.errs

spotless : clean
	- rm ${EXECBIN}

submit : ${SUBMITS}
	submit ${PROJECT} ${SUBMITS}

deps : ${CSOURCE} ${CHEADER}
	@ echo "# ${DEPSFILE} created `date`" >${DEPSFILE}
	${MKDEPS} ${CSOURCE} >>${DEPSFILE}

${DEPSFILE} :
	@ touch ${DEPSFILE}
	${GMAKE} deps

again :
	${GMAKE} spotless deps ci all

ifeq "${NEEDINCL}" ""
include ${DEPSFILE}
endif
