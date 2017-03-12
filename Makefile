#
# FILE: Makefile
#
VERSION=1.1-pre2

#
# Select Highwind or Diablo (http://www.nntp.sol.net/patches/diablo/)
#
# This only changes the timeout in poll to be used by the program
# 
# SERVER = -DDIABLO
SERVER = -DHIGHWIND

#CC = gcc
#CFLAGS = -O2 -c ${SERVER}

CFLAGS = -g -O2 -c ${SERVER} -D_REENTRANT


#
# You shouldn't need to change anything bellow
# 

DEST = ./spam-filtering

multifilter: main.o signal.o article-read.o fd.o parse.o \
	runprg.o init.o version.o func.o filter_t.o pid.o filters.o str.o
	${CC} -o ${DEST}/multifilter main.o signal.o \
	article-read.o fd.o parse.o runprg.o init.o version.o func.o \
	filters.o filter_t.o pid.o str.o -lpthread

article-read.o: article-read.c include/defaults.h include/article-read.h \
	include/func.h include/main.h
	${CC} ${CFLAGS} article-read.c -o article-read.o

filters.o: filters.c include/defaults.h include/filters.h \
	include/fd.h include/func.h include/pid.h include/filter_t.h
	${CC} ${CFLAGS} filters.c -o filters.o 

filter_t.o: filter_t.c include/defaults.h include/filter_t.h \
	include/fd.h include/func.h include/pid.h
	${CC} ${CFLAGS} filter_t.c -o filter_t.o 

fd.o: fd.c include/defaults.h include/fd.h \
	include/func.h include/main.h
	${CC} ${CFLAGS} fd.c -o fd.o

func.o: func.c include/defaults.h include/func.h
	${CC} ${CFLAGS} func.c -o func.o

init.o: init.c include/defaults.h include/init.h \
	include/filter_t.h include/func.h include/main.h \
        include/parse.h include/pid.h include/runprg.h include/signal.h
	${CC} ${CFLAGS} init.c -o init.o

main.o: main.c include/defaults.h include/main.h \
	include/article-read.h include/filter_t.h include/func.h \
	include/init.h include/pid.h
	${CC} ${CFLAGS} main.c -o main.o 

parse.o: parse.c include/defaults.h include/parse.h \
	include/filter_t.h include/fd.h include/func.h include/main.h
	${CC} ${CFLAGS} parse.c -o parse.o

pid.o: pid.c include/defaults.h include/pid.h \
	include/fd.h include/func.h
	${CC} ${CFLAGS} pid.c -o pid.o 

runprg.o: runprg.c include/defaults.h include/runprg.h \
	include/func.h include/filter_t.h
	${CC} ${CFLAGS} runprg.c -o runprg.o

str.o: str.c include/defaults.h
	${CC} ${CFLAGS} str.c -o str.o

signal.o: signal.c include/defaults.h include/signal.h \
	include/filter_t.h include/func.h include/main.h \
	include/parse.h include/runprg.h
	${CC} ${CFLAGS} -D_POSIX_PTHREAD_SEMANTICS signal.c -o signal.o

version.o: version.c include/defaults.h
	${CC} ${CFLAGS} version.c -o version.o

purify:
	purify gcc ${SERVER} -g main.c signal.c fd.c article-read.c \
	runprg.c filters.c init.c version.c filter_t.c parse.c func.c pid.c \
	str.c -o multifilter-pure -lpthread

distrib:
	rm -f *.o ./${DEST}/multifilter multifilter-pure \
	./${DEST}/sample_filter*
	cp ../DistribFiles/* .
	(cd ../ ; tar -cf multifilter-${VERSION}.tar multifilter-${VERSION} ; \
	gzip -9 multifilter-${VERSION}.tar )

clean:
	rm -f *.o
