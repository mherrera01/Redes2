#Macros definition
CC = gcc
CFLAGS = -g -Wall
CHILOS = -pthread
CCONFUSE = -lconfuse

OBJDIR = obj
LIBDIR = lib
OBJECTS = $(OBJDIR)/pool_thread.o $(OBJDIR)/server.o
LIBS = $(LIBDIR)/socketlib.a $(LIBDIR)/picolib.a

#We specify that the files are not related with the rules, so that the makefile will not search for a file called as a rule
.PHONY: all clean createDirs

all: clean createDirs server

createDirs:
	mkdir -p $(OBJDIR)
	mkdir -p $(LIBDIR)

server: $(OBJECTS) $(LIBS)
	$(CC) $(CFLAGS) $(CHILOS) -o$@ $^ $(CCONFUSE)

$(LIBDIR)/socketlib.a: $(OBJDIR)/socket.o
	ar -rv $@ $^

$(LIBDIR)/picolib.a: $(OBJDIR)/picohttpparser.o
	ar -rv $@ $^

#The $< symbol refers to the name of the .c to compile
$(OBJDIR)/socket.o: srclib/socket.c includes/socket.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/pool_thread.o: srclib/pool_thread.c includes/pool_thread.h
	$(CC) $(CFLAGS) $(CHILOS) -c $< -o $@

$(OBJDIR)/server.o: src/server.c
	$(CC) $(CFLAGS) -c $< -o $@ $(CCONFUSE)

$(OBJDIR)/picohttpparser.o: srclib/picohttpparser.c includes/picohttpparser.h
	$(CC) $(CFLAGS) -c $< -o $@

runv_s:
	valgrind --leak-check=full ./server

run_s:
	./server

clean:
	rm -rvf $(OBJDIR)
	rm -rvf $(LIBDIR)
	rm -rf server log.txt