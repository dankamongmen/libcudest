.DELETE_ON_ERROR:
.PHONY: all test clean
.DEFAULT_GOAL:=all

OUT:=out
INC:=$(wildcard src/*.h)
SRC:=$(notdir $(wildcard src/*.c))
SHIMSRC:=$(notdir $(wildcard shim/*.c))
OBJ:=$(addsuffix .o,$(addprefix $(OUT)/,$(basename $(SRC))))
SHIMOBJ:=$(addsuffix .o,$(addprefix $(OUT)/,$(basename $(SHIMSRC))))
CTXSRC:=bin/cudactx.c
DEVSRC:=bin/cudadevices.c
MINSRC:=bin/cudaminimal.c
LIB:=$(OUT)/libcudest.so $(OUT)/shim.so
TAGBIN:=ctags
TAGS:=.tags

NVSRC?=nv/current

CC?=gcc
IFLAGS:=-Isrc -I$(NVSRC)
CFLAGS:=-pipe -g -ggdb -D_GNU_SOURCE -std=gnu99 $(IFLAGS) -fpic -Wall -W -Werror -march=native -mtune=native

BIN:=$(OUT)/cudactx $(OUT)/cudactx-base $(OUT)/cudadevices $(OUT)/cudaminimal $(OUT)/cudaminimal-base

all: $(LIB) $(BIN)

$(TAGS): $(addprefix src/,$(SRC)) $(addprefix shim/,$(SHIMSRC)) $(INC)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(TAGBIN) -f $@ $^

$(OUT)/cudactx: $(CTXSRC) $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(OUT) -Wl,-R$(OUT) -lcudest -lcuda

$(OUT)/cudactx-base: $(CTXSRC) $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -lcuda

$(OUT)/cudadevices: $(DEVSRC) $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(OUT) -Wl,-R$(OUT) -lcuda

$(OUT)/cudaminimal: $(MINSRC) $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(OUT) -Wl,-R$(OUT) -lcudest -lcuda

$(OUT)/cudaminimal-base: $(MINSRC)
	$(CC) $(CFLAGS) -o $@ $< -lcuda

$(OUT)/shim.so: $(SHIMOBJ)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LFLAGS) -ldl

$(OUT)/libcudest.so: $(OBJ)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LFLAGS)

$(OUT)/%.o: shim/%.c $(INC) $(TAGS)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/%.o: src/%.c $(INC) $(TAGS)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

test: all $(BIN)
	./$(OUT)/cudadevices
	./$(OUT)/cudactx 0
	./$(OUT)/cudaminimal-base
	./$(OUT)/cudaminimal

clean:
	rm -rf $(OUT)
