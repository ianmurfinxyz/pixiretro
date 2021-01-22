
LDLIBS = -lSDL2 -lm -lGLX_mesa
CXXFLAGS = -g -Wall -std=c++17 -fno-exceptions

CXXSRC = engine.cpp input.cpp bmpimage.cpp gfx.cpp filerc.cpp log.cpp
CXXINC = engine.h input.h bmpimage.h gfx.h filerc.h log.h app.h utility.h color.h math.h

example : $(CXXSRC) $(CXXINC)
	$(CXX) $(CXXSRC) $(CXXFLAGS) -o $@  $(LDLIBS)

.PHONY: clean
clean:
	rm si *.o
