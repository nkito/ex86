CC      =  ia16-elf-gcc
OBJCOPY	= objcopy

SOURCES = \
	bios.c \
	basicio.c \
	terminal.c \
	timer.c \
	system.c \
	intr.c \
	int10h.c \
	int13h.c \
	int14h.c \
	int15h.c \
	int16h.c \
	int1ah.c \
	emu_interface.c

TARGET        = bios.hex
TARGET_LST    = $(patsubst %.hex,%.lst,$(TARGET))
TARGET_BIN    = $(patsubst %.hex,%.bin,$(TARGET))

SRC_DIR   = src
SRCS      = $(addprefix $(SRC_DIR)/,$(SOURCES))
BUILDDIR  = obj
OBJS      = $(addprefix $(BUILDDIR)/,$(patsubst %.c,%.s,$(SOURCES)))

CFLAGS   += -march=i8086 -Wall -I$(SRC_DIR) -O2

all: $(TARGET)

$(TARGET): reset_entry.s crt.s $(OBJS) intr_asm.s 
	$(CC) $(CFLAGS)  -Wa,-adhln $^ -nostdlib  -lgcc -T link.ld -o $(TARGET) > $(TARGET_LST)
	objcopy -I ihex -O binary $(TARGET) $(TARGET_BIN)

$(BUILDDIR)/%.s : $(SRC_DIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC)  -S $(CFLAGS) $< -o $@ 

clean: 
	-rm $(TARGET) $(TARGET_BIN) $(TARGET_LST) $(OBJS)

disasm:
	ia16-elf-objdump -b ihex -D -mi8086 $(TARGET)

