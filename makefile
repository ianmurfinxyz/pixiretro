
LDLIBS = -lSDL2 -lm -lGLX_mesa
CXXFLAGS = -g -Wall -std=c++17 -fno-exceptions -DTINYXML2_DEBUG

CXXSRC = engine.cpp input.cpp bmpimage.cpp gfx.cpp filerc.cpp log.cpp example.cpp tinyxml2.cpp
CXXINC = engine.h input.h bmpimage.h gfx.h filerc.h log.h app.h utility.h color.h math.h tinyxml2.h

example : $(CXXSRC) $(CXXINC)
	$(CXX) $(CXXSRC) $(CXXFLAGS) -o $@  $(LDLIBS)

.PHONY: clean
clean:
	rm si *.o
