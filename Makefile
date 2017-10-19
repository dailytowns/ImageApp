# commento

# definizione delle variabili
CC=gcc
INC_DIR=include

CFLAGS=-g -Wall -Wextra `mysql_config --cflags` `MagickWand-config --cflags` `xml2-config --cflags` -I$(INC_DIR)
XMLFLAGS = -g `xml2-config --cflags` `mysql_config --cflags` -I$(INC_DIR)

CFILES=$(shell ls *.c)
PROGS=$(CFILES:%.c=%)

LIBS=`mysql_config --libs` `MagickWand-config --libs` `xml2-config --libs`
XMLLIBS=`mysql_config --libs` `xml2-config --libs`

#all: $(PROGS)

httpserver: HttpServer.c Strings.c HandleDB.c ThreadPool.c HandleImage.c Message.c Utils.c Log.c
	$(CC) $(CFLAGS) $? $(LIBS) -o $@

treexml: ParseWurfl.c Utils.c
	$(CC) $(XMLFLAGS) $? $(XMLLIBS) -o $@ > fff

# %: %.c
# 	$(CC) $(CFLAGS) -o $@ $^ 

clean:
	rm -f $(PROGS) *.o
