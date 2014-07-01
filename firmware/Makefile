BINARY = main
LDSCRIPT = libopencm3/lib/stm32/l1/stm32l15xx6.ld

PREFIX	?= arm-none-eabi
CC		= $(PREFIX)-gcc
LD		= $(PREFIX)-gcc
OBJCOPY		= $(PREFIX)-objcopy
OBJDUMP		= $(PREFIX)-objdump
GDB		= $(PREFIX)-gdb

TOOLCHAIN_DIR   ?= libopencm3
ARCH_FLAGS      = -mthumb -mcpu=cortex-m3 -msoft-float
CFLAGS		+= -O0 -ggdb -g3 \
		   -Wall -Wextra -Wimplicit-function-declaration \
		   -Wredundant-decls -Wstrict-prototypes \
		   -Wundef -Wshadow \
		   -I$(TOOLCHAIN_DIR)/include \
		   -std=c11 \
		   -fno-common $(ARCH_FLAGS) -MD -DSTM32L1
LDFLAGS		+= -static -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group \
		   -L$(TOOLCHAIN_DIR)/lib \
		   -T$(LDSCRIPT) -nostartfiles -Wl,--gc-sections \
		   $(ARCH_FLAGS) -mfix-cortex-m3-ldrd

CFLAGS          += -I$(TOOLCHAIN_DIR)/include
LDFLAGS         += -L$(TOOLCHAIN_DIR)/lib -L$(TOOLCHAIN_DIR)/lib/stm32/l1
SCRIPT_DIR      = $(TOOLCHAIN_DIR)/share

OBJS		+= $(BINARY).o usart.o clock.o charge.o leds.o adc.o timeout.o

OOCD		?= openocd
OOCD_INTERFACE	?= flossjtag
OOCD_BOARD	?= olimex_stm32_h103

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
NULL := 2>/dev/null
else
LDFLAGS += -Wl,--print-gc-sections
endif

.SUFFIXES: .elf .bin .hex .srec .list .images
.SECONDEXPANSION:
.SECONDARY:

all: images

images: $(BINARY).images
flash: $(BINARY).flash

%.images: %.bin %.hex %.srec %.list
	@#echo "*** $* images generated ***"

%.bin: %.elf
	@printf "  OBJCOPY $(*).bin\n"
	$(Q)$(OBJCOPY) -Obinary $(*).elf $(*).bin

%.hex: %.elf
	@printf "  OBJCOPY $(*).hex\n"
	$(Q)$(OBJCOPY) -Oihex $(*).elf $(*).hex

%.srec: %.elf
	@printf "  OBJCOPY $(*).srec\n"
	$(Q)$(OBJCOPY) -Osrec $(*).elf $(*).srec

%.list: %.elf
	@printf "  OBJDUMP $(*).list\n"
	$(Q)$(OBJDUMP) -S $(*).elf > $(*).list

%.elf: $(OBJS) $(LDSCRIPT) $(TOOLCHAIN_DIR)/lib/libopencm3_stm32l1.a
	@printf "  LD      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(LD) -o $(*).elf $(OBJS) -lopencm3_stm32l1 $(LDFLAGS)

%.o: %.c Makefile
	@printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(Q)rm -f *.o
	$(Q)rm -f *.d
	$(Q)rm -f *.elf
	$(Q)rm -f *.bin
	$(Q)rm -f *.hex
	$(Q)rm -f *.srec
	$(Q)rm -f *.list

.PHONY: images clean

-include $(OBJS:.o=.d)
