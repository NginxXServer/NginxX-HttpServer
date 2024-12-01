CC = gcc
CFLAGS = -Wall -O -g -pthread -I./server -I./server/tpool
LDFLAGS = -pthread

bin = http_server

t1 = main
document_src = document/document
parser_src = http/parser
server_src = server/server
logger_src = logger/logger
tpool_src = server/tpool/tpool
objs = $(t1).o $(document_src).o $(parser_src).o $(server_src).o $(logger_src).o $(tpool_src).o

all: $(bin)

$(bin): $(objs)
	$(CC) $(CFLAGS) $(objs) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(bin) *.o document/*.o http/*.o server/*.o logger/*.o server/tpool/*.o
