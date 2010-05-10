.DELETE_ON_ERROR:
.PHONY: all test clean
.DEFAULT_GOAL:=test

OUT:=out
INC:=$(wildcard src/*.h)
TEXOBJS:=$(wildcard texobjs/*)
SRC:=$(notdir $(wildcard src/*.c))
SHIMSRC:=$(notdir $(wildcard shim/*.c))
OBJ:=$(addsuffix .o,$(addprefix $(OUT)/,$(basename $(SRC))))
SHIMOBJ:=$(addsuffix .o,$(addprefix $(OUT)/,$(basename $(SHIMSRC))))
MINSRC:=bin/cudaminimal.c
SPAWNSRC:=bin/cudaspawner.c
LIB:=$(OUT)/libcudest.so $(OUT)/shim.so
TAGBIN:=ctags
TAGS:=.tags

CC?=gcc
IFLAGS:=-Isrc
CFLAGS:=-pipe -g -ggdb -D_GNU_SOURCE -std=gnu99 $(IFLAGS) -fpic -Wall -W -Werror -march=native -mtune=native

BIN:=$(OUT)/cudaminimal $(OUT)/cudaminimal-base $(OUT)/cudaspawner

all: $(LIB)

$(TAGS): $(addprefix src/,$(SRC)) $(addprefix shim/,$(SHIMSRC)) $(INC)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(TAGBIN) -f $@ $^

$(OUT)/cudaminimal: $(MINSRC) $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(OUT) -Wl,-R$(OUT) -lcudest -lcuda

$(OUT)/cudaminimal-base: $(MINSRC)
	$(CC) $(CFLAGS) -o $@ $< -lcuda

$(OUT)/cudaspawner: $(SPAWNSRC) $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(OUT) -Wl,-R$(OUT) -lcudest -lcuda

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

%.pdf: %.tex %.bib $(TEXOBJS) $(MAKEFILES)
	@[ -d $(@D) ] || mkdir -p $(@D)
	pdflatex $<
	bibtex $(basename $<)
	pdflatex $<
	pdflatex $<

test: all $(BIN)
	./$(OUT)/cudaminimal-base
	./$(OUT)/cudaminimal
	#./$(OUT)/cudaspawner 0 0

clean:
	rm -rf $(OUT)
