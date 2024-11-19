CC = gcc
CFLAGS = -Wall -O -g

SRC_DIR = src/version1
DOC_DIR = $(SRC_DIR)/document
HTTP_DIR = $(SRC_DIR)/http
SERVER_DIR = $(SRC_DIR)/server
UTILS_DIR = $(SRC_DIR)/utils

TARGET = http_server
T1 = main
OBJ = $(DOC_DIR)/document.o $(HTTP_DIR)/parser.o $(SERVER_DIR)/server.o $(T1).o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)