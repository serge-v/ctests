TARGETS=\
	server-c \

all: $(TARGETS)

server-c: server.c
	gcc -g -o server-c server.c -lrt
clean:
	rm $(TARGETS)
