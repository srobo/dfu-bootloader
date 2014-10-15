PREFIX = arm-none-eabi
CC = $(PREFIX)-gcc
LD = $(PREFIX)-gcc
SIZE = $(PREFIX)-size
GDB = $(PREFIX)-gdb
OBJCOPY = $(PREFIX)-objcopy
OOCD = openocd

LDSCRIPT = stm32.ld
OOCD_BOARD = usb_dfu.cfg

CFLAGS += -mcpu=cortex-m3 -mthumb -msoft-float -DSTM32F1 \
	  -Wall -Wextra -Os -std=gnu99 -g -fno-common \
	  -Ilibopencm3/include
LDFLAGS += -lc -lm -Llibopencm3/lib \
	   -Llibopencm3/lib/stm32/f1 -lnosys -T$(LDSCRIPT) \
	   -nostartfiles -Wl,--gc-sections,-Map=dfu.map -mcpu=cortex-m3 \
	   -mthumb -march=armv7-m -mfix-cortex-m3-ldrd -msoft-float

O_FILES = usbdfu.o

all: usb_dfu.bin

include depend

usb_dfu.elf: $(O_FILES) $(LD_SCRIPT)
	$(LD) -o $@ $(O_FILES) $(LDFLAGS) -lopencm3_stm32f1
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
	-rm -f usb_dfu.elf depend *.o