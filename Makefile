CC = gcc

TARGET = build/chatfilter.so

SRCS = chatfilter.c

CFLAGS = -m32 -O3 -Wall -shared -fPIC

INCLUDES = -I. -I..

LDFLAGS = -m32 -lm

all: prepare $(TARGET)

prepare:
	@mkdir -p build

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "\nSuccessful! Plugin available: $(TARGET)\n"
	
clean:
	rm -rf build