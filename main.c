// Annoy-o-tron by XDjackieXD https://chaosfield.at/projects/annoyotron.html
// A small little device that beeps randomly every 0.5-3.6 hours for a few hundred milliseconds.
// connect piezo to PB0 and a small 3V lithium battery to an ATTiny13, hide it at your friend's place and have fun :P
// The ATTiny runs at 128kHz so the lower fusebit has to be 0x7b and higher fusebit 0xfd

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define LFSR_SEED       (91)
#define OP_START_BEEP   (0)
#define OP_STOP_BEEP    (1)

//#define DEBUG

static uint8_t cnt8 = LFSR_SEED;
static uint8_t prng_lfsr8(void) {
    return (cnt8 = (cnt8 >> 1) ^ (-(cnt8 & 1) & 0xB4));
}

uint16_t random_time() {
    // 30 min to ~3.6h
    return (1800 + ((uint16_t) prng_lfsr8()) * 38) / 8;
}

void start_speaker() {
    // Our speaker is on PWM 0 so we need it as output
    DDRB |= _BV(PB0);
#ifdef DEBUG
    PORTB |= _BV(PB3);
#endif
    TCCR0B |= _BV(CS00);
}

void stop_speaker() {
#ifdef DEBUG
    PORTB &= ~_BV(PB3);
#endif
    TCCR0B &= ~_BV(CS00);
    DDRB &= ~_BV(PB0);

}

void wait_sleep() {
    // WDTCR
    sleep_enable();
    //WDTCR |= _BV(SM1);
    //WDTCR &= ~_BV(SM0);;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_cpu();
}

void wait_idle() {
    // Prescale = 32k
    sleep_enable();
    //WDTCR |= _BV(SM0) | _BV(SM1);
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_cpu();
}


void wd_short() {
    reset_wd();
    WDTCR |= _BV(WDP2);
}

void reset_wd() {
    WDTCR |= _BV(WDTIE);
    WDTCR &= ~(_BV(WDP3) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0));
}

void wd_long() {
    reset_wd();
#ifndef DEBUG
    WDTCR |= _BV(WDP3) | _BV(WDP0);
#endif
}

int main() {

#ifdef DEBUG
    DDRB |= _BV(PB3); // Debug LED
#endif

    // Disable Analog Comparator
    ACSR &= ~_BV(ACIE);
    ACSR |= _BV(ACD);

    // PWM
    TCCR0A |= _BV(COM0A0);
    TCCR0A |= _BV(WGM01);
    OCR0A = 3;

    // Globally enable interrupts
    sei();

    // Beep once to show that everything is working
    start_speaker();
    wd_short();
    wait_idle();
    stop_speaker();

    // Main loop
    for (;;) {
        uint16_t time = random_time();
        for (uint16_t i = 0; i < time; i++) {
            wd_long();
            wait_sleep();
        }
        start_speaker();
        wd_short();
        wait_idle();
        stop_speaker();
    }
}

// Nothing to do here
ISR(WDT_vect) {}
