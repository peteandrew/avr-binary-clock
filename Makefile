PROJECT = bin_clock

MCU = attiny2313
F_CPU = 1000000

CC=avr-gcc
OBJCOPY=avr-objcopy
AVRDUDE = avrdude

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -Os -I.
CFLAGS += -Wall -Wstrict-prototypes
CFLAGS += -std=gnu99


all: $(PROJECT).hex

$(PROJECT).hex: $(PROJECT).elf 
	$(OBJCOPY) -R .eeprom -O ihex $(PROJECT).elf $(PROJECT).hex 

$(PROJECT).elf: $(PROJECT).c
	$(CC) $(CFLAGS) $(PROJECT).c -o $(PROJECT).elf

$(PROJECT).lst: $(PROJECT).elf
	$(OBJCOPY) -S $(PROJECT).elf $(PROJECT).lst

flash: $(PROJECT).hex
	$(AVRDUDE) -P /dev/ttyACM0 -c stk500v2 -b 19200 -p $(MCU) -U flash:w:$(PROJECT).hex

clean:
	rm -f *.hex *.elf *.lst
