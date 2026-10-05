CPP = g++


NORMAL = server client
MULTI_IO = multi_server multi_client
SAFE_MULTI = safe_server safe_client

PROGS = ${NORMAL} ${MULTI_IO} ${SAFE_MULTI}

all: ${PROGS}

normal: ${NORMAL}

multi: ${MULTI_IO}

safe: ${SAFE_MULTI}

server: server.cpp
	${CPP} -o $@  $<

multi_server: multi_server.cpp
	${CPP} -o $@  $<

safe_server: safe_server.cpp
	${CPP} -o $@  $<



client: client.cpp
	${CPP} -o $@  $<

multi_client: multi_client.cpp
	${CPP} -o $@  $<

safe_client: safe_client.cpp
	${CPP} -o $@  $<

clean:
	rm -f ${PROGS}
