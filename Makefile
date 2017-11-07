# commento

# definizione delle variabili
CC=gcc
INC_DIR=include

CFLAGS=-Wall -Wextra -pg `mysql_config --cflags` `MagickWand-config --cflags` `xml2-config --cflags` -I$(INC_DIR)
XMLFLAGS = -g `xml2-config --cflags` `mysql_config --cflags` -I$(INC_DIR)
CURLFLAGS = -g -Wall -Wextra `curl-config --cflags`

LIBS=`mysql_config --libs` `MagickWand-config --libs` `xml2-config --libs`
XMLLIBS=`mysql_config --libs` `xml2-config --libs`
CURLLIBS = `curl-config --libs`


#all: $(PROGS)

httpserver: HttpServer.c HandleImage.c Utils.c Strings.c HandleDB.c ThreadPool.c HandleImage.c Utils.c Log.c Request.c Config.c
	$(CC) $(CFLAGS) $? $(LIBS) -o $@

treexml: ParseWurfl.c Utils.c
	$(CC) $(XMLFLAGS) $? $(XMLLIBS) -o $@ > fff

testcurl: TestCurl.c
	$(CC) $(CFLAGS) $? $(LIBS) -o $@

timer: TimerHandler.c Strings.c HandleDB.c ThreadPool.c HandleImage.c Message.c Utils.c Log.c
	$(CC) $(CFLAGS) $? $(LIBS) -o $@

# %: %.c
# 	$(CC) $(CFLAGS) -o $@ $^ 

clean:
	rm -f $(PROGS) *.o
