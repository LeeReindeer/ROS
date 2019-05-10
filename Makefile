CC=avr-gcc
OBJCOPY=avr-objcopy
SIZE=avr-size
DUDE=/home/leer/program/arduino-1.8.9/hardware/tools/avr/bin/avrdude

MCU=atmega328p
FCPU=16000000L

CFLAGS= -g -Wall -Werror -Os -mmcu=$(MCU) -DF_CPU=$(FCPU) -DARDUINO=10809 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR

DUDE_CONF=/home/leer/program/arduino-1.8.9/hardware/tools/avr/etc/avrdude.conf
BUILD_DIR=build

TARGET=ros
SOURCE=$(wildcard *.c)
# *.c -> build/*.o
OBJS=$(addprefix $(BUILD_DIR)/,$(SOURCE:.c=.o)

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build elf, depends on *.o
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	@echo Building $@...
	$(CC) $(CFLAGS) $(OBJS) -o $@
	$(SIZE) -C --mcu=$(MCU) $@

# Build hex, depends on elf
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo Building $@...
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

upload:
	@echo Upload ROS to board...
	$(DUDE) -C $(DUDE_CONF) -v -p $(MCU) -c arduino -P /dev/ttyACM0 -b 115200 -D -U flash:w:$(BUILD_DIR)/$(TARGET).hex:i 

.PHONY: clean
clean:
	rm -rf build