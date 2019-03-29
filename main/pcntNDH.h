#ifndef PCNTNDH_H_INCLUDED
#define PCNTNDH_H_INCLUDED

#define PCNT_TEST_UNIT      PCNT_UNIT_0
#define PCNT_INPUT_SIG_IO   4  // Pulse Input GPIO
#define PCNT_INPUT_CTRL_IO  5  // Control GPIO HIGH=count up, LOW=count down

typedef struct {
    int unit;  // the PCNT unit that originated an interrupt
    uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;

void IRAM_ATTR pcnt_intr_handler(void *arg);

void pcnt_init(void);

double water_flow(double limit);

#endif
