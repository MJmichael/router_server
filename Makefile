CC = gcc
CFLAGS = -g -I./
#LDFLAGS = -lCppUTest

obj := parser
all: $(obj) 
#test_rbtree

$(obj): parser.o router_dev.o
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

clean:
	-rm -f *.o $(obj)

