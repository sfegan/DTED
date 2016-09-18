include Makefile.common

LIBOBJECTS = DTED.o

OBJECTS = $(LIBOBJECTS)

TARGETS = libDTED.a load_srtm find_flat map

LIBS =  -lDTED -lPhysics -lVSUtility -lmysqlclient -lz

all: $(TARGETS)

libDTED.a: $(LIBOBJECTS)
	$(AR) r $@ $^

load_srtm: load_srtm.o libDTED.a 
	$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

find_flat: find_flat.o libDTED.a 
	$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

map: map.o libDTED.a 
	$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

.PHONY: clean

clean:
	$(RM) \
	$(TARGETS) $(OBJECTS) *~ test

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<
