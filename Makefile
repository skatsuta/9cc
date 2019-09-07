CFLAGS = -std=c11 -g -static
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
BIN = 9cc

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

.PHONY: test
test:
	docker container run --mount type=bind,source=$(PWD),target=/src,consistency=delegated \
		-w /src gcc make test-linux

.PHONY: test-linux
test-linux: $(BIN)
	./test.sh

.PHONY: clean
clean:
	rm -f $(BIN) *.o *~ tmp*
