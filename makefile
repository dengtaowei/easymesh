
# include config.mk
CC = gcc
ECHO = echo

SUB_DIR = 	util/ \
			reactor/ \
			timer/ \
			mbus/ \
			mbus_cli/ \
			agent/ \
			test/ \
			cfg_mgr/

ROOT_DIR = $(shell pwd)
OBJS_DIR = $(ROOT_DIR)/objs
BIN_DIR = $(ROOT_DIR)/bin

BIN = mesh_agent mbusd mbus_cli cfg_mgr test

# LIB = 

FLAG = 	-I$(ROOT_DIR)/include/ \
		-I$(ROOT_DIR)/reactor/ \
		-I$(ROOT_DIR)/timer/ \
		-I$(ROOT_DIR)/mbus/ \
		-Wall -g




LDFLAGS = 	-lpthread



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
		$(OBJS_DIR)/minheap.o \
		$(OBJS_DIR)/eloop_event.o \
		$(OBJS_DIR)/mh-timer.o \
		$(OBJS_DIR)/bus_msg.o

MBUSD_OBJS =	$(OBJS_DIR)/mbusd.o \
		$(OBJS_DIR)/eloop_event.o \
		$(OBJS_DIR)/minheap.o \
		$(OBJS_DIR)/mh-timer.o \
		$(OBJS_DIR)/list.o \
		$(OBJS_DIR)/group_manager.o \
		$(OBJS_DIR)/user_manager.o \
		$(OBJS_DIR)/bus_msg.o

MBUSCLI_OBJS =	$(OBJS_DIR)/mbus_cli.o \
		$(OBJS_DIR)/eloop_event.o \
		$(OBJS_DIR)/minheap.o \
		$(OBJS_DIR)/mh-timer.o \
		$(OBJS_DIR)/list.o \
		$(OBJS_DIR)/group_manager.o \
		$(OBJS_DIR)/user_manager.o \
		$(OBJS_DIR)/bus_msg.o \

ELOOPSERVER_OBJS = $(OBJS_DIR)/eloop_server_test.o \
		$(OBJS_DIR)/eloop_event.o \
		$(OBJS_DIR)/minheap.o \
		$(OBJS_DIR)/mh-timer.o \
		$(OBJS_DIR)/list.o


CFGMGR_OBJS =	$(OBJS_DIR)/cfg_mgr.o \
		$(OBJS_DIR)/cmdu.o \
		$(OBJS_DIR)/core.o \
		$(OBJS_DIR)/ieee1905_network.o \
		$(OBJS_DIR)/list.o \
		$(OBJS_DIR)/tlv_parser.o \
		$(OBJS_DIR)/wsc.o \
		$(OBJS_DIR)/minheap.o \
		$(OBJS_DIR)/eloop_event.o \
		$(OBJS_DIR)/mh-timer.o \
		$(OBJS_DIR)/bus_msg.o \
		$(OBJS_DIR)/util.o

mesh_agent : $(OBJS)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LDFLAGS)

mbusd : $(MBUSD_OBJS)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LDFLAGS)

mbus_cli : $(MBUSCLI_OBJS)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LDFLAGS)

cfg_mgr : $(CFGMGR_OBJS)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LDFLAGS)

test : eloop_server_test eloop_client_test mh-timer_test

eloop_server_test : $(ELOOPSERVER_OBJS)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LDFLAGS)

eloop_client_test : $(OBJS_DIR)/eloop_client_test.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LDFLAGS)

mh-timer_test : $(OBJS_DIR)/mh-timer_test.o $(OBJS_DIR)/minheap.o $(OBJS_DIR)/mh-timer.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LDFLAGS)


clean :
	rm -rf $(BIN_DIR)/* $(OBJS_DIR)/*

