VERS=1.4
NAME=mpigraph-$(VERS)

all:
	cd mpiBench; make
	cd mpiGraph; make

clean:
	cd mpiBench; make clean
	cd mpiGraph; make clean

tar:
	rm -rf temptgz $(NAME).tgz; \
        mkdir -p temptgz/$(NAME); \
        mkdir -p temptgz/$(NAME)/mpiBench; \
        mkdir -p temptgz/$(NAME)/mpiGraph; \
        cp makefile README temptgz/$(NAME); \
        pushd mpiBench; cp makefile README mpiBench.c crunch_mpiBench ../temptgz/$(NAME)/mpiBench/.; popd; \
        pushd mpiGraph; cp makefile README mpiGraph.c crunch_mpiGraph ../temptgz/$(NAME)/mpiGraph/.; popd; \
        cd temptgz; \
        tar -zcf ../$(NAME).tgz $(NAME); \
        cd ..; \
        rm -rf temptgz;
	
