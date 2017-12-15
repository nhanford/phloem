
#  Default build variables
CC=mpicc
LD=mpicc
CFLAGS=-O2
LDFLAGS=$(CFLAGS)

#  LLNL-specific run variables
PROCS=3
NODES=2
POOL=pdebug
TARGET_ARGS=--num_cores=$$(($(PROCS)/$(NODES))) --num_nbors=2 #--num_iters=4096 --win_size=1 --max_msgsize=4096


TARGET=sqmr

all: $(TARGET)

$(TARGET).o : $(TARGET).c
	$(CC) $(CFLAGS) -c $<

$(TARGET) : $(TARGET).o
	$(LD) -o $(TARGET) $(TARGET).o $(LDFLAGS) $(LIBS)

run: $(TARGET)
	srun -n $(PROCS) -N $(NODES) -p$(POOL) ./$(TARGET) $(TARGET_ARGS)
	#poe ./$(TARGET) -procs $(PROCS) -nodes 1 -rmpool pdebug $(TARGET_ARGS)

clean:
	rm -f $(TARGET) $(TARGET).o
