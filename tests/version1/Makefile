CC = gcc
CFLAGS = -Wall -O -g

bin = http_client
src = client_test.c
obj = $(src:.c=.o)

all: $(bin)

$(bin): $(obj)
	$(CC) $(CFLAGS) $(obj) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(bin) $(obj)

