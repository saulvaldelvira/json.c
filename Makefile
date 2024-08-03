.PHONY: libs install uninstall clean

BIN = bin

CFILES = $(shell find . -name '*.c')
OFILES = $(patsubst %.c,%.o,$(CFILES))

AR = ar
ARFLAGS = rcs

CC ?= cc
CFLAGS += -Wall -Wextra -pedantic -std=c11 -Wstrict-prototypes -g -O3 -fPIC

INSTALL_PATH ?= /usr/local

libs: $(OFILES) | $(BIN)/
	@ echo " => $(BIN)/libjson.so"
	@ echo " => $(BIN)/libjson-static.a"
	@ $(CC) $(CCFLAGS) -shared -o ./$(BIN)/libjson.so $(OFILES)
	@ $(AR) $(ARFLAGS) ./$(BIN)/libjson-static.a $(OFILES)

install: $(BIN)/libjson.so $(BIN)/libjson-static.a
	  @ echo "INSTALL $(INSTALL_PATH)/lib/libjson.so"
	  @ echo "INSTALL $(INSTALL_PATH)/lib/libjson-static.a"
	  @ install -d $(INSTALL_PATH)/lib
	  @ install -m 644 $(BIN)/libjson* $(INSTALL_PATH)/lib
	  @ echo "INSTALL $(INSTALL_PATH)/include/json.h"
	  @ install -m 644 $(SRC)/json.h $(INSTALL_PATH)/include/json.h

$(BIN)/libjson.so $(BIN)/libjson-static.a:
	@ make --no-print-directory libs

uninstall:
	  @ echo "RM $(INSTALL_PATH)/lib/libjson.so"
	  @ echo "RM $(INSTALL_PATH)/lib/libjson-static.a"
	  @ rm -f $(INSTALL_PATH)/lib/libjson*
	  @ echo "RM $(INSTALL_PATH)/include/json.h"
	  @ rm -rf $(INSTALL_PATH)/include/json.h

.c.o:
	@ echo " CC $@"
	@ $(CC) $(CFLAGS) -o $@ -c $<

%/:
	@ mkdir $@

clean:
	@ rm -f $(OFILES)
