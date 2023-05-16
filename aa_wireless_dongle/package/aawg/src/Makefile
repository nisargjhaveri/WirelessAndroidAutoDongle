.PHONY: clean
.PHONY: aawgd

EXTRA_CXXFLAGS += $(shell $(PKG_CONFIG) --cflags --libs dbus-cxx-2.0)

aawgd: aawgd.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o '$@' '$<'

clean:
	-rm aawgd
