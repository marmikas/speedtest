CC = gcc

TARGET = speedtest

SRC = src/main.c src/server.c src/speedtest.c src/location.c src/best.c lib/cJSON.c
OBJ = $(SRC:.c=.o)

UNAME_S := $(shell uname -s)

CFLAGS = -Wall -Wextra -Iinc -Ilib
LDFLAGS =
LDLIBS = -lcurl

ifeq ($(UNAME_S),Darwin)
	CURL_PREFIX := $(shell brew --prefix curl)
	CFLAGS += -I$(CURL_PREFIX)/include
	LDFLAGS += -L$(CURL_PREFIX)/lib
endif

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

src/main.o: inc/server.h inc/speedtest.h inc/location.h inc/best.h
src/server.o: inc/server.h lib/cJSON.h
src/speedtest.o: inc/speedtest.h inc/server.h
src/location.o: inc/location.h lib/cJSON.h
src/best.o: inc/best.h inc/server.h
lib/cJSON.o: lib/cJSON.h

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: clean