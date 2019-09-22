CFLAGS = -std=c11 -g -static -fno-common
OS = $(shell uname -s | tr A-Z a-z)
HDRS = $(wildcard *.h)
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
BIN = 9cc
TMP = tmp

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
		--workdir /src \
		gcc make test

# Test command for Linux
.PHONY: test-linux
test-linux: $(BIN)
	./$(BIN) tests > $(TMP).s
	$(CC) -o $(TMP) $(TMP).s
	./$(TMP)

.PHONY: clean
clean:
	rm -f $(BIN) *.o *~ tmp*
