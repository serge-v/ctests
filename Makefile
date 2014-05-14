TARGETS=\
	server-c \

all: $(TARGETS)

server-c: server.c
	$(CC) -v -g -o server-c server.c -lrt
clean:
	rm $(TARGETS)
