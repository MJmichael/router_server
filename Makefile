#CC = $(CC) 
CFLAGS = -g -I./ -I../src -I../apmib -DWIFI_SIMPLE_CONFIG -DUNIVERSAL_REPEATER -DWLAN_WPA -DWLAN_WDS -DWLAN_8185AG -DWLAN_WPA2
LDFLAGS = -lapmib -L../apmib 

obj := server 
all: $(obj) 
#test_rbtree

$(obj): parser.o router_dev.o main.o fmwlan.o
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

clean:
	-rm -f *.o $(obj)

