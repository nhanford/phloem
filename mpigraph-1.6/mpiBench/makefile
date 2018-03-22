# for linux
CC=mpicc
CFLAGS=-O3

#this should agree with the VERS char string in mpiBench.c
VERS=1.6
NAME=mpiBench-$(VERS)

# for AIX
#CC=mpxlc

mpiBench : clean
	$(CC) $(CFLAGS) -o mpiBench mpiBench.c

check : clean
	$(CC) $(CFLAGS) -D CHECK_ALWAYS -o mpiBench mpiBench.c

nobar : clean
	$(CC) $(CFLAGS) -D NO_BARRIER -o mpiBench mpiBench.c

debug : clean
	$(CC) $(CFLAGS) -g -O0 -o mpiBench mpiBench.c

clean :
	rm -f mpiBench ; rm -f *.o

tar: tgz
tgz:
	rm -rf temptgz $(NAME).tgz; \
	mkdir -p temptgz/$(NAME); \
	cp makefile README mpiBench.c crunch_mpiBench temptgz/$(NAME)/.; \
	cd temptgz; \
	tar -zcf ../$(NAME).tgz $(NAME); \
	cd ..; \
	rm -rf temptgz;
