# Author: Adam Henry, adlhenry@ucsc.edu
# $Id: Makefile,v 1.1 2015-03-26 14:33:42-07 - - $

MKFILE    = Makefile
DEPSFILE  = ${MKFILE}.deps
NOINCLUDE = ci clean spotless
NEEDINCL  = ${filter ${NOINCLUDE}, ${MAKECMDGOALS}}
GMAKE     = gmake --no-print-directory
GCC       = g++ -g -O0 -Wall -Wextra -std=gnu++0x
MKDEPS    = g++ -MM -std=gnu++0x

CSOURCE   = main.cpp auxlib.cpp lyutils.cpp stringset.cpp astree.cpp
CHEADER   = auxlib.h lyutils.h stringset.h astree.h
LSOURCE   = scanner.l
YSOURCE   = parser.y
CLGEN     = yylex.cpp
HYGEN     = yyparse.h
CYGEN     = yyparse.cpp
CGENS     = ${CLGEN} ${CYGEN}
ALLGENS   = ${HYGEN} ${CGENS}
ALLCSRC   = ${CSOURCE} ${CGENS}
OBJECTS   = ${ALLSRC:.cpp=.o}
EXECBIN   = oc
SOURCES   = ${CHEADER} ${CSOURCE} ${LSOURCE} ${YSOURCE} ${MKFILE} README
SUBMITS   = ${SOURCES}

# Define for current project
PROJECT   = cmps104a-wm.s15 asg2

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${GCC} -o $@ ${OBJECTS}

%.o : %.cpp
	${GCC} -c $<

${CLGEN} : ${LSOURCE}
	flex --outfile=${CLGEN} ${LSOURCE}

${CYGEN} ${HYGEN} : ${YSOURCE}
	bison --defines=${HYGEN} --output=${CYGEN} ${YSOURCE}

ci : ${SOURCES}
	cid + ${SOURCES}
	checksource ${SOURCES}

clean :
	- rm ${OBJECTS} ${ALLGENS} ${DEPSFILE}

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
