CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=804478782

# Add all .cpp files that need to be compiled for your server
SERVER_FILES=server.cpp Packets.cpp TCP.cpp HeaderConverter.cpp

# Add all .cpp files that need to be compiled for your client
CLIENT_FILES=client.cpp Packets.cpp TCP.cpp HeaderConverter.cpp

all: clean server client

*.o: *.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

server: $(SERVER_FILES:.cpp=.o)
	$(CXX) -o $@ $(CXXFLAGS) $(SERVER_FILES:.cpp=.o)

client: $(CLIENT_FILES:.cpp=.o)
	$(CXX) -o $@ $(CXXFLAGS) $(CLIENT_FILES:.cpp=.o)
	rm *.o

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
