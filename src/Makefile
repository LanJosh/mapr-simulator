IDIR=../include
CC=gcc
CFLAGS=-I$(IDIR) -std=c99 -lpthread

ODIR=obj

_DEPS = buffered_queue.h hashmap.h word_count.h
DEPS = $(patsubst %, $(IDIR)/%, $(_DEPS))

_OBJ = buffered_queue.o hashmap.o word_count.o main.o
OBJ = $(patsubst %, $(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~

