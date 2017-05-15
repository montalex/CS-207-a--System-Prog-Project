CC = gcc
CFLAGS += -std=c99 -Wall -pedantic
CFLAGS += -I/usr/local/opt/openssl/include
CFLAGS += $$(pkg-config vips --cflags)
LDFLAGS += -L../libmongoose
LDLIBS += $$(pkg-config vips --libs) -lm -lssl -lcrypto -ljson-c
LDLIBS2 += $$(pkg-config vips --libs) -lm -lssl -lcrypto -ljson-c -lmongoose
EXEC = pictDBM
EXEC2 = pictDB_server
OBJECT = pictDBM.o db_list.o db_utils.o error.o db_create.o db_delete.o dedup.o db_insert.o db_read.o db_gbcollect.o image_content.o pictDBM_tools.o
OBJECT2 = pictDB_server.o db_list.o db_utils.o error.o db_delete.o dedup.o db_insert.o db_read.o image_content.o pictDBM_tools.o

all: $(EXEC) $(EXEC2)

pictDBM: $(OBJECT)
	$(CC) $(CFLAGS) $(OBJECT) -o $(EXEC) $(LDLIBS)

pictDB_server: $(OBJECT2)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECT2) -o $(EXEC2) $(LDLIBS2)

.PHONY: clean mrproper astyle

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
	rm -rf $(EXEC2)
	rm -rf *.orig

astyle:
	astyle -A8 *.c *.h
