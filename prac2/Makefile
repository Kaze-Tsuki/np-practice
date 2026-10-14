CPP = g++
FLAGS = --std=c++20

THREAD_BASIC = thread_basic thread_cli 
THREAD_ADVANCE = thread_advance thread_cli

PROGS = ${THREAD_ADVANCE} ${THREAD_BASIC}

all: ${PROGS}


thread: ${THREAD_BASIC}
thread3: ${THREAD_ADVANCE}


thread_basic: thread_basic.cpp
	${CPP} ${FLAGS} -o $@  $<

thread_advance: thread_advance.cpp
	${CPP} ${FLAGS} -o $@ $<


thread_cli: thread_cli.cpp
	${CPP} ${FLAGS} -o $@ $<

clean:
	rm -f ${PROGS}
