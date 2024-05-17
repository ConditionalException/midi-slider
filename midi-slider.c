#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "bsp/board.h"
#include "tusb.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
const uint8_t DEVIATION = 2;
const uint8_t NUM_SAMPLES = 20;
const uint32_t UPDATE_RATE_MS = 10;

// Values within CLAMP +- end of range will be forced to min or max
const uint8_t CLAMP_RANGE = 2;

const uint8_t MIDI_MIN = 0;
const uint8_t MIDI_MAX = 127;

typedef struct midi_control_t {
  uint8_t value;
  uint8_t channel;
  uint32_t last_update;
} midi_control_t;

void update_midi_control(midi_control_t *, uint8_t);

int main() {
  uint16_t val[3] = {0};
  midi_control_t controls[] = {
      {.value = 0, .channel = 1, .last_update = 0},
      {.value = 0, .channel = 2, .last_update = 0},
      {.value = 0, .channel = 3, .last_update = 0},
  };
  const float conversion_factor = (float)(MIDI_MAX + 1) / (1 << 12);

  stdio_init_all();
  board_init();
  tusb_init();
  adc_init();

  adc_gpio_init(26);
  adc_gpio_init(27);
  adc_gpio_init(28);

  adc_set_clkdiv(0);
  adc_select_input(0);

  while (1) {
    tud_task();
    for (int i = 0; i < 3; i++) {
      adc_select_input(i);
      val[i] = adc_read();
      for (uint8_t c = 0; c < NUM_SAMPLES; c++) {
        val[i] = (val[i] + adc_read()) / 2;
      }
      update_midi_control(&controls[i], val[i] * conversion_factor);
    }
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) { board_led_on(); }

// Invoked when device is unmounted
void tud_umount_cb(void) { board_led_off(); }

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) { board_led_off; }

// Invoked when usb bus is resumed
void tud_resume_cb(void) { board_led_on; }

//--------------------------------------------------------------------+
// MIDI Task
//--------------------------------------------------------------------+

void update_midi_control(midi_control_t *control, uint8_t val) {
  uint8_t msg[3];
  uint8_t difference = 0;

  // Guard from values above MIDI_MAX
  val = val % (MIDI_MAX + 1);

  // Clamp outer range to min or max
  if (val >= (MIDI_MAX - CLAMP_RANGE)) {
    val = MIDI_MAX;
  }
  if (val <= (MIDI_MIN + CLAMP_RANGE)) {
    val = MIDI_MIN;
  }
  difference = ((control->value > val) ? (control->value - val) : (val - control->value));

  if ((difference >= DEVIATION) && (board_millis() - control->last_update) >= UPDATE_RATE_MS) {
    control->last_update = board_millis();
    control->value = val;

    msg[0] = 176; // Control type
    msg[1] = control->channel;
    msg[2] = val;
    tud_midi_n_stream_write(0, 0, msg, 3);
  }
}
