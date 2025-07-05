
# include config.mk
CC = gcc
ECHO = echo

SUB_DIR = 	agent/ \
			util/
ROOT_DIR = $(shell pwd)
OBJS_DIR = $(ROOT_DIR)/objs
BIN_DIR = $(ROOT_DIR)/bin

BIN = mesh_agent

# LIB = 

FLAG = -I$(ROOT_DIR)/include




LDFLAGS = 	-lpthread -O3 \
			-levent



CUR_SOURCE = ${wildcard *.c}
CUR_OBJS = ${patsubst %.c, %.o, $(CUR_SOURCE)}

export CC BIN_DIR OBJS_DIR ROOT_DIR FLAG BIN ECHO EFLAG



all : check_objs check_bin $(SUB_DIR) $(CUR_OBJS) $(LIB) bin
.PHONY : all

bin: $(BIN)

# lib: $(SUB_DIR) $(LIB)

$(SUB_DIR) : ECHO
	make -C $@

$(CUR_OBJS) : %.o : %.c
	$(CC) -c $^ -o $(OBJS_DIR)/$@ $(FLAG)

#DEBUG : ECHO
#	make -C bin

ECHO :
	@echo $(SUB_DIR)

check_objs:
	if [ ! -d "objs" ]; then \
		mkdir -p objs;  \
	fi

check_bin:
	if [ ! -d "bin" ]; then \
		mkdir -p bin;   \
	fi

OBJS = 	$(OBJS_DIR)/cmdu.o \
		$(OBJS_DIR)/core.o \
		$(OBJS_DIR)/ieee1905_network.o \
		$(OBJS_DIR)/list.o \
		$(OBJS_DIR)/main.o \
		$(OBJS_DIR)/tlv_parser.o \
		$(OBJS_DIR)/wsc.o \

mesh_agent : $(OBJS)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LDFLAGS)


clean :
	rm -rf $(BIN_DIR)/* $(OBJS_DIR)/*

