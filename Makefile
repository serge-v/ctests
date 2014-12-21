TARGETS=\
	server-c \
	gl1 \
	gl2

all: $(TARGETS)

server-c: server.c
	$(CC) -v -g -o server-c server.c -lrt -lpthread

odbcload: odbcload.c
	$(CC) -g -Wall -Wextra $< -o $@ -lodbc
	

gl1: gl1.c
	$(CC) -g -Wall -Wextra $< -o $@ -lglut -lGL -lxcb-glx -lXext -lX11 -ldrm -lxcb -lm -lXdmcp -lX11-xcb

gl2: gl2.c
	$(CC) -g -Wall -Wextra $< -o $@ -lglut -lGL -lxcb-glx -lXext -lX11 -ldrm -lxcb -lm -lXdmcp -lX11-xcb -lGLU

clean:
	rm $(TARGETS)
