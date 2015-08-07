#
# wimpy make example to build lab
#
IDIR =./include
CC=gcc
CFLAGS=-I$(IDIR) -g

ODIR=obj
LDIR =./lib

LIBS=-lm

_DEPS = ia32_encode.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = encodeit.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

encodeit: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ encodeit
