CFLAGS = -std=c11 -g -static
OS = $(shell uname -s | tr A-Z a-z)
HDRS = $(wildcard *.h)
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
BIN = 9cc

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(HDRS)

# Dispatch test command depending on the OS
.PHONY: test
test: test-$(OS)

# Test command for macOS
.PHONY: test-darwin
test-darwin:
	docker container run \
		--rm \
		--mount type=bind,source=$(PWD),target=/src,consistency=delegated \
		-w /src gcc make test

# Test command for Linux
.PHONY: test-linux
test-linux: $(BIN)
	./test.sh

.PHONY: clean
clean:
	rm -f $(BIN) *.o *~ tmp*
