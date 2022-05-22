PREFIX = arm-none-eabi
CC = $(PREFIX)-gcc
LD = $(PREFIX)-gcc
SIZE = $(PREFIX)-size
GDB = $(PREFIX)-gdb
OBJCOPY = $(PREFIX)-objcopy
STRIP = $(PREFIX)-strip
OOCD = openocd

# Directory containing include/ and lib/ subdirectories of libopencm3 installation.
LIBOPENCM3 ?= libopencm3

LDSCRIPT = stm32.ld
OOCD_BOARD = usb_dfu.cfg

ifdef SR_BOOTLOADER_VID
ifdef SR_BOOTLOADER_PID
ifdef SR_BOOTLOADER_REV
ifdef SR_BOOTLOADER_FLASHSIZE
IDNUMSAREGOOD=1
endif
endif
endif
endif

ifndef IDNUMSAREGOOD
$(error You must export the VID/PID/REV and flashsize for the bootloader to this make enviroment)
endif

CFLAGS += -mcpu=cortex-m3 -mthumb -msoft-float -DSTM32F1 \
	  -Wall -Wextra -Os -std=gnu99 -g -fno-common \
	  -I$(LIBOPENCM3)/include -DSR_BOOTLOADER_VID=$(SR_BOOTLOADER_VID) \
	  -DSR_BOOTLOADER_PID=$(SR_BOOTLOADER_PID) \
	  -DSR_BOOTLOADER_REV=$(SR_BOOTLOADER_REV) \
	  -DSR_BOOTLOADER_FLASHSIZE=$(SR_BOOTLOADER_FLASHSIZE)
LDFLAGS += -lc -lm -L$(LIBOPENCM3)/lib \
	   -L$(LIBOPENCM3)/lib/stm32/f1 -lnosys -T$(LDSCRIPT) \
	   -nostartfiles -Wl,--gc-sections,-Map=dfu.map -mcpu=cortex-m3 \
	   -mthumb -march=armv7-m -mfix-cortex-m3-ldrd -msoft-float

O_FILES = usbdfu.o boot.o

all: usb_dfu_blob.o usb_dfu.bin

include depend

usb_dfu_blob.o: $(O_FILES) $(LD_SCRIPT) $(FORCE_BOOTLOADER_OBJ)
	if test -z "$$FORCE_BOOTLOADER_OBJ"; then echo "No force_bootloader object provided in environment" 1>&2; exit 1; fi
	$(LD) -r -o $@ $(O_FILES) $$FORCE_BOOTLOADER_OBJ $(LDFLAGS) -lopencm3_stm32f1 '-Wl,-e reset_handler'
	$(OBJCOPY) --redefine-sym reset_handler=bootloader_entry $@
	$(STRIP) -K bootloader_entry $@

usb_dfu.elf: $(O_FILES) $(LD_SCRIPT)
	if test -z "$$FORCE_BOOTLOADER_OBJ"; then echo "No force_bootloader object provided in environment" 1>&2; exit 1; fi
	$(LD) -o $@ $(O_FILES) $$FORCE_BOOTLOADER_OBJ $(LDFLAGS) -lopencm3_stm32f1
	$(SIZE) $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

depend: *.c
	rm -f depend
	for file in $^; do \
		$(CC) $(CFLAGS) -MM $$file -o - >> $@ ; \
	done ;

.PHONY: all test clean flash

flash: usb_dfu.elf
	$(OOCD) -f "$(OOCD_BOARD)" \
	        -c "init" \
	        -c "reset init" \
	        -c "flash write_image erase $< 0" \
	        -c "reset" \
	        -c "shutdown"

debug: usb_dfu.elf
	$(OOCD) -f "$(OOCD_BOARD)" \
	        -c "init" \
	        -c "reset halt" &
	$(GDB)  $^ -ex "target remote localhost:3333" -ex "mon reset halt" && killall openocd

clean:
	-rm -f usb_dfu.elf depend *.o *.map crctool
