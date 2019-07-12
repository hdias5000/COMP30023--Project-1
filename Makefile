##Program Written By Hasitha Dias
##Student ID: 789929
##University/GitLab Username: diasi
##Email: i.dias1@student.unimelb.edu.au
##Last Modified Date: 19/04/2018

##Adapted from Workshop 3
CC=gcc
CFLAGS=-Wall -Wextra -std=gnu99 -I. -O3 -lpthread
OBJ = server.o
EXE = server
##Create .o files from .c files. Searches for .c files with same .o names given
##in OBJ
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)
##Create executable linked file from object files.
$(EXE): $(OBJ)
	gcc -o $@ $^ $(CFLAGS)
##Delete object files
clean:
	/bin/rm $(OBJ)
##Performs clean (i.e. delete object files) and deletes executable
clobber: clean
	/bin/rm $(EXE)
