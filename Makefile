# ---------------------------------------
# CONFIGURATION BEGIN
# ---------------------------------------

# entry point for the program and target name
# got how to read from all files from one directory from here: https://unix.stackexchange.com/questions/697779/how-to-get-all-c-files-and-store-into-a-variable-in-a-makefile
C_SRCS = src/main.c $(wildcard src/ForC/*.c)
CPP_SRCS = src/modules.cpp

# Object files
C_OBJS = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)


# target name
TARGET := project

# Path to your systemc installation
SCPATH = $(SYSTEMC_HOME)

# Additional flags for the compiler
CXXFLAGS := -std=c++14  -I"$(SCPATH)/include" -Iinclude -L"$(SCPATH)/lib" -lsystemc -lm
CFLAGS := -std=c17 -Wall -Wextra -Iinclude


# ---------------------------------------
# CONFIGURATION END
# ---------------------------------------

# Determine if clang or gcc is available
CXX := $(shell command -v g++ || command -v clang++)
ifeq ($(strip $(CXX)),)
    $(error Neither clang++ nor g++ is available. Exiting.)
endif

CC := $(shell command -v gcc || command -v clang)
ifeq ($(strip $(CC)),)
    $(error Neither clang nor g is available. Exiting.)
endif

# Add rpath except for MacOS
UNAME_S := $(shell uname -s)

ifneq ($(UNAME_S), Darwin)
    CXXFLAGS += -Wl,-rpath="$(SCPATH)/lib"
endif


# Default to release build for both app and library
all: debug

# Rule to compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build
debug: CXXFLAGS += -g
debug: $(TARGET)

# Release build
release: CXXFLAGS += -O2
release: $(TARGET)

# Rule to link object files to executable
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) -o $(TARGET)

# clean up
clean:
	rm -f $(TARGET)
	rm -f $(C_OBJS) $(CPP_OBJS)

.PHONY: all debug release clean
