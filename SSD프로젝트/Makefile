TARGETS := ssd testShell
SRC := $(patsubst %,%.c,$(TARGETS))
OBJ := $(patsubst %,%.o,$(TARGETS))


all: $(TARGETS)


$(TARGETS): %: %.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(TARGETS) $(OBJ) nand.txt result.txt