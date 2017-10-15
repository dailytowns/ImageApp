# commento

# definizione delle variabili
CC=gcc
INC_DIR=include
CFLAGS=-g -Wall -Wextra -O0 `mysql_config --cflags` `MagickWand-config --cflags` `xml2-config --cflags` -I$(INC_DIR)
CFILES=$(shell ls *.c)
PROGS=$(CFILES:%.c=%)
LIBS=`mysql_config --libs` `MagickWand-config --libs` `xml2-config --libs`

#all: $(PROGS)

httpserver: HttpServer.c Strings.c HandleDB.c ThreadPool.c HandleImage.c Message.c
	$(CC) $(CFLAGS) $? $(LIBS) -o $@

# %: %.c
# 	$(CC) $(CFLAGS) -o $@ $^ 

clean:
	rm -f $(PROGS) *.o
