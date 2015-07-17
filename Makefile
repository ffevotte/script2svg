TSM_ROOT = /home/H55056/local/applications/libtsm/build/install

CARGS  = -g -std=c++0x -Wall -I ${TSM_ROOT}/include/
LDARGS = -L ${TSM_ROOT}/lib/ -l tsm -Wl,-rpath,${TSM_ROOT}/lib

script2svg: main.cxx
	g++ $(CARGS) $(LDARGS) $< -o $@
