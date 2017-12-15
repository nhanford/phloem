#
#  MPI Bandwidth and Latency Benchmark Makefile
#
#  Modify the CC, LIBS, CFLAGS, LDFLAGS, LIBDIR, and INCDIR values,
#    if necessary, to compile the test applications for the target system
#

CC=mpicc
LIBS=
CFLAGS= -O2 -g -Wall
LDFLAGS= 
INCDIR=

MSG  = com 
EXES = $(MSG) 

SRCS = com.c util.c util.h
OBJS = com.o util.o 

PACKAGE_FILES = Makefile README $(SRCS)

all def: $(EXES)

msg: $(MSG)

%.o : %.c util.h
	$(CC) $(CFLAGS) $(INCDIR) -c $<

com: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

PROCS=16
NODES=2

runbw: com
	srun -n $(PROCS) -N $(NODES) -ppdebug ./com -u -x -b 1048576 -l #-c 5

runlat: com
	srun -n $(PROCS) -N $(NODES) -ppdebug ./com -w Latency -l -o 10000 -c 20

bbw_targets:
	rm -rf bbw-targets
	mkdir bbw-targets
	./manage_targets.py -p nn -e $${NODES} -t $${TPN} > bbw-targets/nn.$${NODES}nodes.$${TPN}.tpn
	./manage_targets.py -e $${NODES} -t $${TPN} > bbw-targets/bi.$${NODES}nodes.$${TPN}.tpn
	./manage_targets.py -p rand_proc -e $${NODES} -t $${TPN} > bbw-targets/rp.$${NODES}nodes.$${TPN}.tpn
	./manage_targets.py -p rand_node -e $${NODES} -t $${TPN} > bbw-targets/rn.$${NODES}nodes.$${TPN}.tpn

clean:
	rm -f $(EXES) *.o tags

tags: *.c *.h
	ctags *

distclean: clean
	rm -f *~ *.out

dist: distclean
	tar cvf presta.tar $(PACKAGE_FILES)
