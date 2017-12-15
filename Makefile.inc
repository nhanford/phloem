
#  Sequoia benchmark packaging defaults
ENV_OBJ = ../presta*/util.o
#CFLAGS = -DPRINT_ENV

#  Modify the following values
CC = mpicc
CFLAGS += -O2 -g 
LD = $(CC)
LDFLAGS =
LIBS += 

