CGLAGS=-Wall -std=gnugg -g
OBJS=cc.o lex.o string.o

cc: $(OBJS)
		$(CC) $(CFLAGS) -o $@ $(OBJS)