OSTD_CXXFLAGS = -O2 -g -std=c++14 -Wall -Wextra -Wshadow -I.

EXAMPLES_OBJ = \
	examples/format \
	examples/listdir \
	examples/range \
	examples/range_pipe \
	examples/signal \
	examples/stream1 \
	examples/stream2

all: examples

examples: $(EXAMPLES_OBJ)

.cc:
	$(CXX) $(CXXFLAGS) $(OSTD_CXXFLAGS) -o $@ $<

test: test_runner
	@./test_runner

clean:
	rm -f $(EXAMPLES_OBJ) test_runner
