CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 -Wno-sign-compare $(CXXOPTIMIZE)
USERID=804478782
CLASSES=HttpMessage.cpp HttpRequest.cpp HttpResponse.cpp UrlParser.cpp 

all:clean  web-server web-client web-server1_1 web-client1_1

web-server: $(CLASSES)
	$(CXX) -o $@ $(CXXFLAGS) $(CLASSES) $@.cpp

web-client: $(CLASSES)
	$(CXX) -o $@  $(CXXFLAGS) $(CLASSES) $@.cpp

web-server1_1: $(CLASSES)
	$(CXX) -o $@  $(CXXFLAGS) $(CLASSES) $@.cpp

web-client1_1: $(CLASSES)
	$(CXX) -o $@  $(CXXFLAGS) $(CLASSES) $@.cpp

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM web-server web-client web-client1_1 web-server1_1 *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
