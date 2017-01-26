LIBAPR_CPPFLAGS = $(shell apr-1-config --cppflags --includes)
LIBEVENT_CPPFLAGS = -I/usr/include
LIBAPR_LIBS = $(shell apr-1-config --link-ld)
LIBEVENT_LIBS = -levent

CC = gcc
CFLAGS = $(LIBAPR_CPPFLAGS) $(LIBEVENT_CPPFLAGS)
LIBS = $(LIBAPR_LIBS) $(LIBEVENT_LIBS)

OBJS= vs_httpd.o
TARGET = vs_httpd

.SUFFIXES: .c .o

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(TARGET)

.PHONY: clean
clean:
	$(RM) *.o $(TARGET)
