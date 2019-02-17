# Top level makefile, the real shit is at src/Makefile

default: all

all:

ifeq (./src/sisdb/out, $(wildcard ./src/sisdb/out))
	cd ./src/sisdb/out && cmake ../ && $(MAKE)  $@ 
else
	cd ./src/sisdb/ && mkdir out && cd out && cmake ../ && $(MAKE)  $@ 
endif

sisdb:

ifeq (./src/sisdb/out, $(wildcard ./src/sisdb/out))
	cd ./src/sisdb/out && cmake ../ && $(MAKE)
else
	cd ./src/sisdb/ && mkdir out && cd out && cmake ../ && $(MAKE)
endif
	cp ./bin/libsisdb.* ../bin
	
test:

ifeq (./test/out, $(wildcard ./test/out))
	cd ./test/out && cmake ../ && $(MAKE)
else
	cd ./test/ && mkdir out && cd out && cmake ../ && $(MAKE)
endif

clean:

	cd ./src/sisdb/ && rm -rf out

	cd ./test/ && rm -rf out