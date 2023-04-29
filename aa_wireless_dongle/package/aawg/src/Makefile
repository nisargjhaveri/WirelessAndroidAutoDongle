.PHONY: clean
.PHONY: aawgd

EXTRA_CXXFLAGS += $(shell $(PKG_CONFIG) --cflags dbus-cxx-2.0)

aawgd: aawgd.c
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o '$@' '$<'

clean:
	-rm aawgd
