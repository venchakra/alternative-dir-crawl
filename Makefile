CXX= g++
CXXFLAGS= -Wall -g -std=c++11 -pthread
INCLUDE = .
SOURCES=dir_crawl.cc dir_crawl_helper.cc WorkQueue.cc
OBJECTS=$(addsuffix .o, $(basename $(SOURCES)))

all: dircrawl

dircrawl: $(OBJECTS)
	  $(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ -I$(INCLUDE)

clean:
	rm -f $(OBJECTS) dircrawl
	rm -rf *~
