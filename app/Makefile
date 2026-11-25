# Basic setup
SRC_DIR     = src
INC_DIR     = include
OPENCM3_DIR = ../lib/libopencm3
BINARY      = firmware

# Device configuration
LIBNAME     = opencm3_stm32f4
DEFS        = -DSTM32F4 -DDEBUG
FP_FLAGS    = -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS  = -mthumb -mcpu=cortex-m4 $(FP_FLAGS)

# Linker script
LDSCRIPT = linkerscript.ld

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
LD      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

# Source files (only firmware.c)
# OBJS = $(SRC_DIR)/firmware.o

# Update source files
SRC_FILES = firmware.c usb_setup.c 
# SRC_FILES =  usb_hid.c 
OBJS = $(addprefix $(SRC_DIR)/,$(SRC_FILES:.c=.o))
# Compiler flags
CFLAGS = -Os -std=c99 -ggdb3
CFLAGS += $(ARCH_FLAGS)
CFLAGS += -Wextra -Wshadow -Wimplicit-function-declaration
CFLAGS += -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
CFLAGS += -fno-common -ffunction-sections -fdata-sections
CFLAGS += -MD -Wall -Wundef $(DEFS)
CFLAGS += -I$(OPENCM3_DIR)/include
CFLAGS += -I$(INC_DIR)

# Linker flags
LDFLAGS = --static -nostartfiles
LDFLAGS += -T$(LDSCRIPT)
LDFLAGS += $(ARCH_FLAGS) -ggdb3
LDFLAGS += -Wl,-Map=$(BINARY).map -Wl,--gc-sections
LDLIBS = -L$(OPENCM3_DIR)/lib -l$(LIBNAME) 
LDLIBS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group
# Build rules
all: $(BINARY).elf

$(BINARY).elf: $(OBJS) $(LDSCRIPT)
	@echo "  LD      $@"
	$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "  CC      $<"
	$(CC) $(CFLAGS) -o $@ -c $<

$(OPENCM3_DIR)/lib/lib$(LIBNAME).a:
	@echo "  Building libopencm3..."
	$(MAKE) -C $(OPENCM3_DIR)

run:
	@echo " Running"
	bear -- make 
	rm -f $(SRC_DIR)/../../compile_commands.json 
	mv compile_commands.json ../

clean:
	@echo "  CLEAN"
	rm -f $(BINARY).* $(SRC_DIR)/*.o $(SRC_DIR)/*.d

flash: $(BINARY).elf
	@echo "  FLASH   $<"
	st-flash write $(BINARY).bin 0x08000000

%.bin: %.elf
	@echo "  OBJCOPY $@"
	$(OBJCOPY) -Obinary $< $@

.PHONY: all clean flash

-include $(SRC_DIR)/*.d
