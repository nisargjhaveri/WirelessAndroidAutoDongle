.PHONY: clean

EXTRA_CXXFLAGS += $(shell $(PKG_CONFIG) --cflags --libs dbus-cxx-2.0)

aawgd: aawgd.o bluetoothHandler.o
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o '$@' $^

%.o: %.cpp *.h
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -c -o '$@' '$<'

clean:
	-rm aawgd
