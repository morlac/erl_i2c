# 
# Makefile for erl_i2c Erlang C-Node
#
# (c) Copyright by Christian Adams <morlac@morlac.de>
#

CC = gcc

.SUFFIXES: .o .c .h

OFLAGS = -O2

ERL_CC_FLAGS = -I/usr/lib/erlang/lib/erl_interface-3.7.8/include
ERL_LD_FLAGS = -L/usr/lib/erlang/lib/erl_interface-3.7.8/lib
ERL_LD_LIBS = -lerl_interface -lei

CC_FLAGS = $(ERL_CC_FLAGS) $(OFLAGS) -Wall -I./include
LD_FLAGS = $(ERL_LD_FLAGS)
LD_LIBS = $(ERL_LD_LIBS) -lnsl -lpthread -I./include

OBJECTS = erl_i2c_cnode.o

all: erl_i2c_cnode

erl_i2c_cnode.o: erl_i2c_cnode.c

erl_i2c_cnode: $(OBJECTS)
	@$(CC) $(LD_FLAGS) -o $(@) $(OBJECTS) $(LD_LIBS) ;\
		echo -e "\t[LINK]\t$(@)\t{$(?)}"

.c.o:
	@$(CC) $(CC_FLAGS) -g $(<) -o $(@) -c && echo -e "\t[CC]\t$(@)\t{$(?)}" \
		|| echo -e "\t !! ERROR !! target $(@) input $(?)"

install:
	install -D erl_i2c_cnode ../priv/cbin/erl_i2c_cnode

clean:
	@rm -f $(OBJECTS); echo -e "\t[RM]\t$(OBJECTS)"
	@rm -f erl_i2c_cnode; echo -e "\t[RM]\terl_i2c_conde"

