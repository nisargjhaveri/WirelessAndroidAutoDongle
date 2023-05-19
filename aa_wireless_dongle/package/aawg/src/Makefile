.PHONY: clean

EXTRA_CXXFLAGS += $(shell $(PKG_CONFIG) --cflags --libs dbus-cxx-2.0 protobuf-lite)

aawgd: aawgd.o bluetoothHandler.o proto/WifiInfoRequest.pb.o proto/WifiStartRequest.pb.o
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o '$@' $^

%.o: %.cpp *.h proto/*.h
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -c -o '$@' '$<'

%.pb.o: %.pb.cc %.pb.h
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -c -o '$@' '$<'

proto/%.pb.cc proto/%.pb.h: proto/%.proto
	cd $(<D) && $(PROTOC) --cpp_out=. $*.proto

clean:
	-rm aawgd
