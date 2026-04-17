PRU_CGT ?= /usr/share/ti/cgt-pru
PRU_SSP ?= /usr/lib/ti/pru-software-support-package

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJ_NAME   := $(notdir $(patsubst %/,%,$(dir $(MKFILE_PATH))))
GEN_DIR     := gen

INCLUDE := --include_path=$(PRU_CGT)/include \
           --include_path=$(PRU_SSP)/include \
           --include_path=$(PRU_SSP)/include/am335x \
           --include_path=..

CFLAGS  := -v3 -O2 --display_error_number --endian=little --hardware_mac=on \
           --obj_directory=$(GEN_DIR) --pp_directory=$(GEN_DIR) -ppd -ppa   \
           --asm_directory=$(GEN_DIR) --list_directory=$(GEN_DIR)

LFLAGS  := --reread_libs --warn_sections --stack_size=0x100 --heap_size=0x100

TARGET  := $(GEN_DIR)/$(PROJ_NAME).out
OBJECTS := $(patsubst %.c,$(GEN_DIR)/%.object,$(wildcard *.c))

all: $(TARGET)

$(TARGET): $(OBJECTS) AM335x_PRU.cmd
	$(PRU_CGT)/bin/clpru $(CFLAGS) -z \
	    -i$(PRU_CGT)/lib -i$(PRU_CGT)/include \
	    $(LFLAGS) -o $@ $(OBJECTS) -m$(GEN_DIR)/$(PROJ_NAME).map \
	    AM335x_PRU.cmd --library=libc.a

$(GEN_DIR)/%.object: %.c
	mkdir -p $(GEN_DIR)
	$(PRU_CGT)/bin/clpru --include_path=$(PRU_CGT)/include $(INCLUDE) \
	    $(CFLAGS) -fe $@ $<

install: $(TARGET)
	cp $(TARGET) /lib/firmware/$(PROJ_NAME)

clean:
	rm -rf $(GEN_DIR)

.PHONY: all install clean
-include $(OBJECTS:.object=.pp)
