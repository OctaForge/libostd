EXAMPLES_CXXFLAGS = -std=c++14 -Wall -Wextra -Wshadow -I.

EXAMPLES_OBJ = \
	examples/format.o \
	examples/listdir.o \
	examples/range.o \
	examples/signal.o \
	examples/stream1.o \
	examples/stream2.o

examples: $(EXAMPLES_OBJ)

.cc.o:
	$(CXX) $(CXXFLAGS) $(EXAMPLES_CXXFLAGS) -o $@ $<

clean:
	rm -f $(EXAMPLES_OBJ)