
SHELL = /bin/sh

TESTS = presta sqmr mpigraph
BUILD_DIRS = $(TESTS) 

include Makefile.inc

FLAGS=CC="$(CC)" CFLAGS="$(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS)" 


build: 
	@for dir in $(BUILD_DIRS) ; do  \
	  BENCH_DIR=`echo $$dir*` ; \
	  if cd $$BENCH_DIR ; then \
	    echo ================================================================================; \
	    echo "Building benchmark" $$BENCH_DIR ; \
	    echo ================================================================================; \
	    if [ $$dir != "presta" ] ; then \
	      if ! $(MAKE) $(FLAGS) LIBS="$(LIBS) $(ENV_OBJ)"; then exit 1 ; fi ; \
	    else \
	      if ! $(MAKE) $(FLAGS) LIBS=$(LIBS); then exit 1 ; fi ; \
	    fi ;\
	    cd .. ; \
	  fi ; \
	done ; \
	echo ================================================================================; \
	echo "Done." ; \
	echo ================================================================================; 

run: build
	@./run_script $(TESTS)

commands: 
	./run_script -l $(TESTS)

clean:
	for dir in $(BUILD_DIRS) ; do  \
	  if cd $$dir* ; then \
	    $(MAKE) clean ; \
	    cd .. ; \
	  fi ; \
	done

clobber: clean
	rm -f mpi_bench_tests*.out

# ==============================================================================
#   LLNL Release Information
# ------------------------------------------------------------------------------
# Sequoia MPI Benchmark Suite Framework
# Published 1/15/08
# by Chris Chambreau (chambreau1@llnl.gov)
# Lawrence Livermore National Laboratory
# Release number LLNL-MI-400479
