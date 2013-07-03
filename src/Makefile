#
# Makefile
#
#

CXX=g++

#TARGET_BASE=apery
TARGET_BASE=sfentobonanza
ifeq ($(OS),Windows_NT)
	TARGET=${TARGET_BASE}.exe
else
	TARGET=${TARGET_BASE}
endif

CPPSRCS=main.cpp bitboard.cpp init.cpp mt64bit.cpp position.cpp move.cpp \
	square.cpp usi.cpp generateMoves.cpp evaluate.cpp \
	hand.cpp \
	pieceScore.cpp
CPPOBJECTS=${CPPSRCS:.cpp=.o}
LDFLAGS=-lpthread
OPT=-Wall -std=c++0x
#OPT+= -Winline

release:
	$(MAKE) CPPFLAGS='$(OPT) -O3 -DNDEBUG' Apery

assert:
	$(MAKE) CPPFLAGS='$(OPT) -O3' Apery

sse:
	$(MAKE) CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2' LDFLAGS='$(LDFLAGS) -flto' Apery

sse41:
	$(MAKE) CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -msse4.1' Apery

sse42:
	$(MAKE) CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE42 -msse4.2' Apery

debug:
	$(MAKE) CPPFLAGS='$(OPT) -O3 -DNDEBUG -g' Apery

prof:
	$(MAKE) CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2 -pg' LDFLAGS='$(LDFLAGS) -flto' Apery

profgen:
	$(MAKE) CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2 -fprofile-generate -lgcov' LDFLAGS='$(LDFLAGS) -flto -fprofile-generate -lgcov' Apery

profuse:
	$(MAKE) CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2 -fprofile-use -lgcov' LDFLAGS='$(LDFLAGS) -flto -fprofile-use -lgcov' Apery

asm: $(CPPSRCS)
	$(CXX) $(CPPSRCS) $(OPT) -O3 -DNDEBUG -DHAVE_SSE42 -msse4.2 -S

icc:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -DNDEBUG -ip' LDFLAGS='$(LDFLAGS)' Apery

icc-assert:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -ip' LDFLAGS='$(LDFLAGS)' Apery

icc-debug:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -DNDEBUG -ip -g' LDFLAGS='$(LDFLAGS)' Apery

icc-sse:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2 -ip' LDFLAGS='$(LDFLAGS)' Apery

icc-sse41:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -msse4.1 -ip' LDFLAGS='$(LDFLAGS)' Apery

icc-sse42:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE42 -msse4.2 -ip' LDFLAGS='$(LDFLAGS)' Apery

icc-ipo:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -DNDEBUG -ipo' LDFLAGS='$(LDFLAGS) -ipo' Apery

icc-sse-ipo:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2 -ipo' LDFLAGS='$(LDFLAGS) -ipo' Apery

icc-sse-ipo-profgen:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2 -ipo -prof-gen=srcpos -prof_dir ./profdir' LDFLAGS='$(LDFLAGS) -ipo -prof-gen=srcpos -prof_dir ./profdir' Apery

icc-sse-ipo-profuse:
	$(MAKE) CXX=icpc CPPFLAGS='$(OPT) -O3 -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2 -ipo -prof_use -prof_dir ./profdir' LDFLAGS='$(LDFLAGS) -ipo -prof_use -prof_dir ./profdir' Apery


Apery: ${CPPOBJECTS}
	$(CXX) $(CPPOBJECTS) $(CPPFLAGS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f ${CPPOBJECTS} ${TARGET} ${CPPSRCS:.cpp=.gcda}

depend:
	@$(CXX) -MM $(OPT) $(CPPSRCS) > .depend

-include .depend
