TARGETS=\
	server-c \
	odbcload \

all: $(TARGETS)

server-c: server.c
	$(CC) -v -g -o server-c server.c -lrt

odbcload: odbcload.c
	$(CC) -g -Wall -Wextra $< -o $@ -lodbc

clean:
	rm $(TARGETS)
