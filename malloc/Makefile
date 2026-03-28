CFLAGS=-g

%.o: %.c allocator.h alloccommon.h
	$(CC) $(CFLAGS) -c -o $@ $<

alloc: alloc.o debug.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	$(RM) *.o alloc
