CC = gcc
CFLAGS = -I/usr/local/include -I/home/apache/include -Wall -g
LIBS = -L/usr/lib -L/usr/local/lib -levent -L/home/apache/lib -lapr-1
EXEC = vs_httpd

main : vs_httpd.o
	$(CC) $(LIBS) -o $(EXEC) $<

vs_httpd.o : vs_httpd.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	rm -rf *.o $(EXEC)
