src = $(wildcard src/*.c src/Parser/*c)
obj = $(src:.c=.o)

LDFLAGS = -ledit -lm

tertius: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) tertius
