#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "soc/gpio_sig_map.h"

#include "pcntNDH.h"

xQueueHandle pcnt_evt_queue; 
pcnt_isr_handle_t user_isr_handle = NULL;

void IRAM_ATTR pcnt_intr_handler(void *arg) {
    uint32_t intr_status = PCNT.int_st.val;
    int i;
    pcnt_evt_t evt;
    portBASE_TYPE HPTaskAwoken = pdFALSE;

    for (i = 0; i < PCNT_UNIT_MAX; i++) {
        if (intr_status & (BIT(i))) {
            evt.unit = i;
            /* Save the PCNT event type that caused an interrupt
               to pass it to the main program */
            evt.status = PCNT.status_unit[i].val;
            PCNT.int_clr.val = BIT(i);
            xQueueSendFromISR(pcnt_evt_queue, &evt, &HPTaskAwoken);
            if (HPTaskAwoken == pdTRUE) {
                portYIELD_FROM_ISR();
            }
        }
    }
}

/* Initialize PCNT functions:
 *  - configure and initialize PCNT
 *  - set up the input filter
 *  - set up the counter events to watch
 */
void pcnt_init(void) {
    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = PCNT_INPUT_SIG_IO,
        .ctrl_gpio_num = PCNT_INPUT_CTRL_IO,
        .channel = PCNT_CHANNEL_0,
        .unit = PCNT_TEST_UNIT,
        // What to do on the positive / negative edge of pulse input?
        .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
        .neg_mode = PCNT_COUNT_DIS,   // Keep the counter value on the negative edge
        // What to do when control input is low or high?
        .lctrl_mode = PCNT_MODE_REVERSE, // Reverse counting direction if low
        .hctrl_mode = PCNT_MODE_KEEP,    // Keep the primary counter mode if high
    };
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    /* Configure and enable the input filter */
    pcnt_set_filter_value(PCNT_TEST_UNIT, 100);
    pcnt_filter_enable(PCNT_TEST_UNIT);

    /* Initialize PCNT's counter */
    pcnt_counter_pause(PCNT_TEST_UNIT);
    pcnt_counter_clear(PCNT_TEST_UNIT);

    /* Register ISR handler and enable interrupts for PCNT unit */
    pcnt_isr_register(pcnt_intr_handler, NULL, 0, &user_isr_handle);
    pcnt_intr_enable(PCNT_TEST_UNIT);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(PCNT_TEST_UNIT);
}



double water_flow(double limit) {
  pcnt_evt_queue = xQueueCreate(10, sizeof(pcnt_evt_t));
  pcnt_init();

  int16_t count = 0;
  float total_water = 0;

  pcnt_evt_t evt;
  
  while (1) {
	xQueueReceive(pcnt_evt_queue, &evt, 500 / portTICK_PERIOD_MS);
	pcnt_get_counter_value(PCNT_TEST_UNIT, &count);    
	total_water +=  (count / 5880.0) * 1000.0;
	pcnt_counter_clear(PCNT_TEST_UNIT);
	if (totalMilliLitres > limit) {
	  break;
	}
  }
  	
  if(user_isr_handle) {
	esp_intr_free(user_isr_handle);
	user_isr_handle = NULL;
  }
  pcnt_isr_handler_remove(PCNT_TEST_UNIT);
  return totalMilliLitres;
}
