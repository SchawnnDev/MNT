# Directories

SRC_DIR = src
IPT_DIR = input
OBJ_DIR = obj
BIN_DIR = bin
ARCH_DIR = dist

# Program

EXECUTABLE_NAME = mnt
EXECUTABLE = $(BIN_DIR)/./$(EXECUTABLE_NAME)

# Compiler

CC = mpicc
CFLAGS = -Wall -O3 -march=native -g -fopenmp
# CFLAGS=-Wall -O1 -g -fopenmp

# Files and folders

SRCS = $(shell find $(SRC_DIR) -name '*.c')
SRC_DIRS = $(shell find $(SRC_DIR) -type d | sed 's/$(SRC_DIR)/./g' )
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Compiling

$(BIN_DIR)/$(EXECUTABLE_NAME) : title tips build_dir $(OBJS)
	@echo "\n> Compiling : "
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run

run: title tips
	@./bin/$(EXECUTABLE_NAME)

mini: title tips
	@./bin/$(EXECUTABLE_NAME) $(IPT_DIR)/mini.mnt

small: title tips
	@./bin/$(EXECUTABLE_NAME) $(IPT_DIR)/small.mnt

medium: title tips
	@./bin/$(EXECUTABLE_NAME) $(IPT_DIR)/medium.mnt

large: title tips
	@./bin/$(EXECUTABLE_NAME) $(IPT_DIR)/large.mnt

# Utils

build_dir:
	@$(call make-obj)

clean:
	@echo "> Cleaning :"
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(ARCH_DIR)

dist: clean
	@mkdir -p $(ARCH_DIR)
	@echo "> Archiving :"
	tar -czvf $(ARCH_DIR)/MNT-Projet_PP.tar.gz Makefile README.md $(SRC_DIR)

# Functions

# Create obj directory structure
define make-obj
	mkdir -p $(OBJ_DIR)
	for dir in $(SRC_DIRS); \
	do \
		mkdir -p $(OBJ_DIR)/$$dir; \
	done
endef

# Others

list:
	@echo "> List of commands :"
	@echo "make -> compiles the program"
	@echo "make run -> runs the program"
	@echo "make mini -> runs the program with the 'mini' input file"
	@echo "make small -> runs the program with the 'small' input file"
	@echo "make medium -> runs the program with the 'medium' input file"
	@echo "make large -> runs the program with the 'large' input file"
	@echo "make clean -> clears the directory"
	@echo "make dist -> creates an archive"

title:
	@echo ' ______ ______   ________   _________'
	@echo '|\   _ \  _   \|\   ___  \|\___   ___\'
	@echo '\ \  \\\__\ \  \ \  \\ \  \|___ \  \_|'
	@echo ' \ \  \\|__| \  \ \  \\ \  \   \ \  \'
	@echo '  \ \  \    \ \  \ \  \\ \  \   \ \  \'
	@echo '   \ \__\    \ \__\ \__\\ \__\   \ \__\'
	@echo '    \|__|     \|__|\|__| \|__|    \|__|'

tips:
	@echo "> You may want to run <make> before starting the program :)"
	@echo "> Tips : type <make list> to check out the list of commands\n"
