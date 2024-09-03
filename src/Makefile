# Makefile
# This is a comment

# Define the compiler and the flags
CC = arm-buildroot-linux-gnueabihf-gcc
CFLAGS = -o

# Define the target executable and source files
TARGET = test
SRCS = main.c game.c uart.c

# The rule to build the target executable
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(TARGET) $(SRCS) -lts

# The rule to clean the build
clean:
	rm -f $(TARGET)
