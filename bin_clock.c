// TODO:
// Increase tick time to one second and decrease to a tenth when setting
// Turn off display after period of time

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define MOSI         PIND2
#define CLK          PIND4
#define CS           PIND3
#define MODE         PINB0
#define INCR         PINB1


volatile uint8_t tenthTick = 0;


ISR(TIMER1_COMPA_vect)
{
    tenthTick = 1;
}


void delay_ms(int8_t ms)
{
    for (int8_t i=0; i<ms; i++) {
        _delay_ms(1);
    }
}


void spi_init(void)
{
    DDRD |= (1 << MOSI | 1 << CS | 1 << CLK);
    PORTD &= ~(1 << MOSI | 1 << CLK | 1 << CS);
}


void spi_send_byte(unsigned char data)
{
    for (unsigned char i = 0; i < 8; i++) {
        if (data & 0x80) {
            PORTD |= (1 << MOSI);
        } else {
            PORTD &= ~(1 << MOSI);
        }

        _delay_us(2);
        
        // set clock high
        PORTD |= (1 << CLK);
        _delay_us(4);

        // set clock low
        PORTD &= ~(1 << CLK);
        _delay_us(4);

        data <<= 1;
    }
}


void set_register_data(int8_t reg, int8_t data) {
    PORTD &= ~(1 << CS);
    _delay_us(10);
    spi_send_byte(reg);
    spi_send_byte(data);
    _delay_us(10);
    PORTD |= (1 << CS);
    _delay_us(10);
}


void set_row(int8_t row, int8_t val) {
    set_register_data(row, val);
}


void clear_rows(void) {
    for (int8_t i=1; i<9; i++) {
       set_row(i, 0); 
    }
}


int main(void)
{
    /* Set up buttons */
    // Enable pull-ups
    DDRB &= ~(1 << MODE | 1 << INCR);
    PORTB |= (1 << MODE | 1 << INCR);

    /* Set up SPI */
    spi_init();

    /* Set up timer */
    // CTC
    TCCR1B |= (1 << WGM12);
    // Divide by 8 prescaler
    TCCR1B |= (1 << CS11);
    // Output compare A match interrupt enable
    TIMSK |= (1 << OCIE1A);
    // Output compare value of 12500 0x30D4 (1 tenth second)
    OCR1AH = 0x30;
    OCR1AL = 0xD4;

    int8_t intensity = 0x00;

    int8_t tenths = 0;
    int8_t secs = 0;
    int8_t mins = 0;
    int8_t hours = 0;
    int8_t date = 1;
    int8_t month = 1;
    int8_t year = 0;

    /* 0 - normal time, 1 - set time, 2 - dispay blanked */
    int8_t mode = 0;

    int8_t modePressedTenths = 0;
    int8_t incrPressedTenths = 0;

    int8_t set = 1;
    int8_t toggleTenths = 0;

    delay_ms(1);

    // Enable display
    set_register_data(0x0C, 0x01);

    // Set scan limit to maximum
    set_register_data(0x0B, 0x07);

    // Set intensity
    set_register_data(0x0A, intensity);

    clear_rows();

    /* Enable interrupts */
    sei();

    while(1) {
        if (!tenthTick) {
            set_sleep_mode(SLEEP_MODE_IDLE);
            sleep_mode();
            continue;
        }

        // Don't run clock if we're in set mode
        if (mode != 1) {
            tenths++;
            if (tenths > 9) {
                secs++;
                tenths = 0;
            }
            if (secs > 59) {
                mins++;
                secs = 0;
            }
            if (mins > 59) {
                hours++;
                mins = 0;
            }
            if (hours > 23) {
                date++;
                hours = 0;
            }
            if ((year % 4 == 0 && month == 2 && date > 29) ||
                    (year % 4 != 0 && month == 2 && date > 28) ||
                    ((month == 4 || month == 6 || month == 9 || month == 11) && date > 30) ||
                    date > 31) {
                month++;
                date = 1;
            }
            if (month > 12) {
                year++;
                month = 1;
            }
        }

        if (bit_is_clear(PINB, MODE)) {
            modePressedTenths++;
        } else {
            modePressedTenths = 0;
        }

        if (bit_is_clear(PINB, INCR)) {
            incrPressedTenths++;
        } else {
            incrPressedTenths = 0;
        }

        if (mode == 0) {
            set_row(8, secs);
            set_row(7, mins);
            set_row(6, hours);
            set_row(4, date);
            set_row(3, month);
            set_row(2, year);

            if (modePressedTenths > 4) {
                mode = 1;
                modePressedTenths = 0;
            }
            if (incrPressedTenths > 4) {
                mode = 2;
                incrPressedTenths = 0;
                // Shutdown display
                set_register_data(0x0C, 0x00);
            }
        } else if (mode == 1) {
            if (set == 7 && toggleTenths < 4) {
                set_row(8, secs | (1 << 7));
            } else {
                set_row(8, secs);
            }
            if (set == 6 && toggleTenths < 4) {
                set_row(7, mins | (1 << 7));
            } else {
                set_row(7, mins);
            }
            if (set == 5 && toggleTenths < 4) {
                set_row(6, hours | (1 << 7));
            } else {
                set_row(6, hours);
            }
            if (set == 4 && toggleTenths < 4) {
                set_row(4, date | (1 << 7));
            } else {
                set_row(4, date);
            }
            if (set == 3 && toggleTenths < 4) {
                set_row(3, month | (1 << 7));
            } else {
                set_row(3, month);
            }
            if (set == 2 && toggleTenths < 4) {
                set_row(2, year | (1 << 7));
            } else {
                set_row(2, year);
            }
            if (set == 1 && toggleTenths < 4) {
                set_row(1, intensity | (1 << 7));
            } else {
                set_row(1, intensity);
            }

            toggleTenths++;
            if (toggleTenths > 9) toggleTenths = 0;

            if (modePressedTenths > 4) {
                set++;
                if (set > 7) {
                    set = 1;
                    mode = 0;
                    // Clear intensity row
                    set_row(1, 0);
                }
                modePressedTenths = 0;
            }

            if (incrPressedTenths > 2) {
                switch (set) {
                    case 7:
                        secs++;
                        if (secs > 59) {
                            secs = 0;
                        }
                        break;
                    case 6:
                        mins++;
                        if (mins > 59) {
                            mins = 0;
                        }
                        break;
                    case 5:
                        hours++;
                        if (hours > 23) {
                            hours = 0;
                        }
                        break;
                    case 4:
                        date++;
                        if ((year % 4 == 0 && month == 2 && date > 29) ||
                                (year % 4 != 0 && month == 2 && date > 28) ||
                                ((month == 4 || month == 6 || month == 9 || month == 11) && date > 30) ||
                                date > 31) {
                            date = 1;
                        }
                        break;
                    case 3:
                        month++;
                        if (month > 12) {
                            month = 1;
                        }
                        break;
                    case 2:
                        year++;
                        if (year > 99) {
                            year = 0;
                        }
                        break;
                    case 1:
                        intensity++;
                        if (intensity > 0x04) {
                            intensity = 0;
                        }
                        set_register_data(0x0A, intensity);
                        break;
                }
                incrPressedTenths = 0;
            }
        } else if (mode == 2) {
            if (incrPressedTenths > 4) {
                mode = 0;
                incrPressedTenths = 0;
                // Enable display
                set_register_data(0x0C, 0x01);
            }
        }

        tenthTick = 0;

        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_mode();
    }

    return 0;
}
