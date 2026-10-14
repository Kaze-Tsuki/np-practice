CPP = g++
FLAGS = --std=c++20

THREAD_BASIC = thread_basic client 
THREAD_IMPROVE = thread_improve client
THREAD_ADVANCE = thread_advance client

PROGS = ${THREAD_ADVANCE} ${THREAD_IMPROVE} ${THREAD_BASIC}

all: ${PROGS}


thread1: ${THREAD_BASIC}
thread2: ${THREAD_IMPROVE}
thread3: ${THREAD_ADVANCE}


thread_basic: thread_basic.cpp
	${CPP} ${FLAGS} -o $@  $<

thread_improve: thread_improve.cpp
	${CPP} ${FLAGS} -o $@ $<

thread_advance: thread_advance.cpp
	${CPP} ${FLAGS} -o $@ $<


client: thread_client.cpp
	${CPP} ${FLAGS} -o $@ $<

clean:
	rm -f ${PROGS}
