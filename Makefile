all:DAQ eros_d2r Linearity Noise SNR CrossT TRes TCal

setup.o: setup.cxx
		g++  -g -O2 -Wall -pthread -m64 $(shell root-config --cflags --glibs)  -c -o setup.o setup.cxx

rbcp.o: RBCP/rbcp.cxx 
		g++  -g -O2 -Wall -pthread -m64 $(shell root-config --cflags --glibs)  -c -o RBCP/rbcp.o RBCP/rbcp.cxx

DAQ.o: DAQ.cxx 
		g++  -g -O2 -Wall -pthread -m64 $(shell root-config --cflags --glibs)  -c -o DAQ.o DAQ.cxx

DAQ: rbcp.o  setup.o DAQ.o
		g++ -g -O2 -Wall $(shell root-config --cflags)  -o DAQ  RBCP/rbcp.o setup.o DAQ.o $(shell root-config --glibs)

eros_d2r: eros_d2r.cxx
		g++ -g -O2 -Wall $(shell root-config --cflags) -o eros_d2r eros_d2r.cxx $(shell root-config --glibs)

Linearity: anaLinearity.cxx
		g++ -g -O2 -Wall $(shell root-config --cflags) -o Linearity anaLinearity.cxx $(shell root-config --glibs)

Noise: anaNoise.cxx
		g++ $(shell root-config --cflags) -o Noise anaNoise.cxx $(shell root-config --glibs)

SNR: anaSN.cxx
		g++ $(shell root-config --cflags) -o SNR anaSN.cxx $(shell root-config --glibs)

CrossT: anaCrossTalk.cxx
		g++ $(shell root-config --cflags) -o CrossT anaCrossTalk.cxx $(shell root-config --glibs)

TRes: anaTRes.cxx
		g++ $(shell root-config --cflags) -o TRes anaTRes.cxx $(shell root-config --glibs)

TCal: anaTC.cxx
		g++ -g -O2 -Wall $(shell root-config --cflags) -o TCal anaTC.cxx $(shell root-config --glibs)

clean:
		rm -f *.o
		rm -f ./RBCP/*.o
		rm -f DAQ eros_d2r Linearity Noise SNR CrossT TRes TCal

