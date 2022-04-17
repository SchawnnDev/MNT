# Arguments

threads ?= 1
ifeq (0,$(filter 0,$(shell expr $(threads)\>=1) $(shell expr $(THR)\>=1)))
   override threads = 1
endif
override THR = $(threads)
THR_ARG = $(THR)

processes ?= 1
ifeq (0,$(filter 0,$(shell expr $(processes)\>=1) $(shell expr $(PRC)\>=1)))
   override processes = 1
endif
override PRC = $(processes)
PRC_ARG = $(PRC)

input ?= none
ifeq ($(input), none)
else
	IPT_ARG = $(input)
endif

output ?= none
ifeq ($(output), none)
else ifeq ($(output), console)
else
	OPT_ARG = $(output)
endif

# Directories

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
OPT_DIR = output
IPT_DIR = input
ARCH_DIR = dist

# Files

MIN_FL = mini
SML_FL = small
MED_FL = medium
LRG_FL = large
FL_EXT = .mnt

# Program

EXECUTABLE_NAME = mnt
EXECUTABLE = $(BIN_DIR)/./$(EXECUTABLE_NAME)

# Compiler

CC = mpicc
CFLAGS = -Wall -O3 -march=native -g -fopenmp
# CFLAGS=-Wall -O1 -g -fopenmp
EXE_FLAGS = -fopenmp

# Files and folders

SRCS = $(shell find $(SRC_DIR) -name '*.c')
SRC_DIRS = $(shell find $(SRC_DIR) -type d | sed 's/$(SRC_DIR)/./g' )
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Compiling

$(BIN_DIR)/$(EXECUTABLE_NAME) : title tips build_dir $(OBJS)
	@echo "\n> Compiling : "
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) $(EXE_FLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build_dir:
	@$(call make-obj)

# Run
run: title tips
ifdef ($(input))
	@echo "> Input :" $(IPT_ARG)
ifeq (0,$(filter $(output),none console))
	@mkdir -p $(shell dirname $(OPT_ARG))
	@echo "> Output :" $(OPT_ARG) "\n"
endif
	@ OMP_NUM_THREADS=$(THR_ARG) mpirun -n $(PRC_ARG) ./bin/$(EXECUTABLE_NAME) $(IPT_ARG) $(OPT_ARG)
else
	@echo "Usage: make run <input> [<output> <threads> <processes>]"
endif

mini: title tips
ifeq ($(output), none)
	$(eval OPT_ARG = $(OPT_DIR)/$(MIN_FL)$(FL_EXT))
endif
	$(eval IPT_ARG = $(IPT_DIR)/$(MIN_FL)$(FL_EXT))
	@echo "> Input :" $(IPT_ARG)
ifneq ($(output), console)
	@mkdir -p $(shell dirname $(OPT_ARG))
	@echo "> Output :" $(OPT_ARG) "\n"
endif
	@ OMP_NUM_THREADS=$(THR_ARG) mpirun -n $(PRC_ARG) ./bin/$(EXECUTABLE_NAME) $(IPT_ARG) $(OPT_ARG)

small: title tips
ifeq ($(output), none)
	$(eval OPT_ARG = $(OPT_DIR)/$(SML_FL)$(FL_EXT))
endif
	$(eval IPT_ARG = $(IPT_DIR)/$(SML_FL)$(FL_EXT))
	@echo "> Input :" $(IPT_ARG)
ifneq ($(output), console)
	@mkdir -p $(shell dirname $(OPT_ARG))
	@echo "> Output :" $(OPT_ARG) "\n"
endif
	@ OMP_NUM_THREADS=$(THR_ARG) mpirun -n $(PRC_ARG) ./bin/$(EXECUTABLE_NAME) $(IPT_ARG) $(OPT_ARG)

medium: title tips
ifeq ($(output), none)
	$(eval OPT_ARG = $(OPT_DIR)/$(MED_FL)$(FL_EXT))
endif
	$(eval IPT_ARG = $(IPT_DIR)/$(MED_FL)$(FL_EXT))
	@echo "> Input :" $(IPT_ARG)
ifneq ($(output), console)
	@mkdir -p $(shell dirname $(OPT_ARG))
	@echo "> Output :" $(OPT_ARG) "\n"
endif
	@ OMP_NUM_THREADS=$(THR_ARG) mpirun -n $(PRC_ARG) ./bin/$(EXECUTABLE_NAME) $(IPT_ARG) $(OPT_ARG)

large: title tips
ifeq ($(output), none)
	$(eval OPT_ARG = $(OPT_DIR)/$(LRG_FL)$(FL_EXT))
endif
	$(eval IPT_ARG = $(IPT_DIR)/$(LRG_FL)$(FL_EXT))
	@echo "> Input :" $(IPT_ARG)
ifneq ($(output), console)
	@mkdir -p $(shell dirname $(OPT_ARG))
	@echo "> Output :" $(OPT_ARG) "\n"
endif
	OMP_NUM_THREADS=$(THR_ARG) mpirun -n $(PRC_ARG) ./bin/$(EXECUTABLE_NAME) $(IPT_ARG) $(OPT_ARG)


# Utils

clean:
	@echo "> Cleaning :"
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(ARCH_DIR)
	rm -rf $(OPT_DIR)

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
	@echo "make args -> show the arguments available when running"
	@echo "make clean -> clears the directory"
	@echo "make dist -> creates an archive"
	@echo "make run -> runs the program \n\t Usage: make run <input> [<output> <threads> <processes>]"
	@echo "make mini -> runs the program with the 'mini' input file \n\t Usage: make mini [<input> <output> <threads> <processes>]"
	@echo "make small -> runs the program with the 'small' input file \n\t Usage: make small [<input> <output> <threads> <processes>]"
	@echo "make medium -> runs the program with the 'medium' input file \n\t Usage: make medium [<input> <output> <threads> <processes>]"
	@echo "make large -> runs the program with the 'large' input file \n\t Usage: make large [<input> <output> <threads> <processes>]"

args:
	@echo "/!\ Only available for : <make run>, <make mini>, <make small>, <make medium> and <make large> /!\ "
	@echo "> List of arguments :"
	@echo "threads -> number of threads when running with OpenMP, default = 1"
	@echo "processes -> number of processes when running with MPI, default = 1"
	@echo "input -> custom path to the input file, has a default path is set, required for <make run>"
	@echo "output -> custom path for the output file, has a default path is set, \n\t\t output=console to display in the terminal"
	@echo "Example : make run input=input/mini.mnt output=console threads=2 processes=2"


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
