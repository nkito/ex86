CC      = gcc
OBJCOPY	= objcopy

SOURCES = \
	mem.c \
	io.c \
	terminal.c \
	dev_timer.c \
	dev_PIC.c \
	dev_DMAC.c \
	dev_FDC.c \
	dev_UART.c \
	dev_CMOS.c \
	dev_video.c \
	emu_interface.c \
	logfile.c \
	misc.c \
	ALUop.c \
	ExInst86.c \
	ExInst186.c \
	ExInst386.c \
	decode.c \
	descriptor.c \
	mainloop16.c \
	mainloop32.c \
	i8086.c

TARGET    = ex86
SRC_DIR   = src
OBJ_DIR   = obj
SRCS      = $(addprefix $(SRC_DIR)/,$(SOURCES))
OBJS      = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(SOURCES)))

CFLAGS    = -O3 -Wall -g

# If estimation of the byte order during compile time fails or
# you need to specify the byte order such as the cases for cross compiling, 
# please use the corresponding one of following two lines
#
# CFLAGS += -DTARGET_BYTE_ORDER_LITTLE_ENDIAN
# CFLAGS += -DTARGET_BYTE_ORDER_BIG_ENDIAN

all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ 

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ 

clean: 
	-rm $(TARGET) 
	-rm $(OBJS) 

.p.bin:
	p2bin $*.p $*.bin

.bin.dis:
	objdump -b binary -D -mi8086 $*.bin > $*.dis		


