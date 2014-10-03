CFLAGS ?= -DSunOS
CXXFLAGS ?= -DSunOS
CC ?= gcc
CXX ?= g++
CFLAGS += -DMEM_DEBUG -DDEBUG -g -O -Wall -I.
CXXFLAGS += -DMEM_DEBUG -DDEBUG -g -O -Wall -I.
LIBS := -lm
OBJS :=  xjig.o socket.o message.o event.o New.o window.o
XJIG := xjig
RM := /bin/rm -f
MV := /bin/mv
STRIP ?= /usr/bin/strip

PURIFY_FLAGS ?= -collector=/usr/psup/gcc-2.6.0/lib/gcc-lib/sparc-sun-sunos4.1.3/2.6.0/ld g++

all: $(XJIG)

$(XJIG): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $(XJIG) 

install: all
	$(STRIP) $(XJIG)
clean:
	$(RM) $(XJIG) $(OBJS)

xjig.o: system.h socket.h message.h xjig.h event.h New.h window.h List.h List.cc
	$(CXX) $(CXXFLAGS) -c xjig.cc -o xjig.o

socket.o: system.h socket.h New.h socket.cc
	$(CXX) $(CXXFLAGS) -c socket.cc -o socket.o

message.o: system.h socket.h message.h New.h message.cc
	$(CXX) $(CXXFLAGS) -c message.cc -o message.o

event.o: system.h message.h event.h New.h Fifo.h Fifo.cc event.cc
	$(CXX) $(CXXFLAGS) -c event.cc -o event.o

New.o: New.h New.cc
	$(CXX) $(CXXFLAGS) -c New.cc -o New.o

window.o: system.h window.h List.h List.cc message.h window.cc
	$(CXX) $(CXXFLAGS) -c window.cc -o window.o

purify:
	/usr/sup/purify/purify -home=/usr/sup/purify/PureLink-1.1/ -collector=rld/ld -g++ $(CC) $(CFLAGS) -o $(OBJS) $(LIBS)
#	/usr/sup/purify/purify -home=/usr/sup/purify/PureLink-1.1/ -collector=/usr/sup/purify/PureLink-1.1/rld/ld -g++ $(CC) $(CFLAGS) -o $(OBJS) $(LIBS)

Xjunk.o: system.h Xjunk.h Xjunk.cc
	$(CXX) $(CXXFLAGS) -c Xjunk.cc -o Xjunk.o
