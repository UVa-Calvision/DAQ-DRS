CC = g++
CFLAGS = -std=c++14 -pthread -m64 -I/usr/include/root
LIBS = -lCAENDigitizer
ROOTLIBS=-L/usr/lib64/root -lGui -lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lROOTVecOps -lTree -lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -lROOTDataFrame -pthread -lm -ldl -rdynamic


all: dt5742test

clean: 
	rm -f dt5742test

dt5742test: dt5742test.cc
	$(CC) $(CFLAGS) $(LIBS)  dt5742test.cc -o dt5742test $(ROOTLIBS)
