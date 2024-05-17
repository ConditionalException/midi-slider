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

void update_midi_control(uint8_t);

int main() {
  uint16_t val = 0;
  const float conversion_factor = (float)(MIDI_MAX + 1) / (1 << 12);

  stdio_init_all();
  board_init();
  tusb_init();
  adc_init();

  adc_gpio_init(26);
  adc_select_input(0);

  while (1) {
    tud_task();

    val = adc_read();
    for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
      val = (val + adc_read()) / 2;
    }

    update_midi_control(val * conversion_factor);
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

void update_midi_control(uint8_t val) {
  uint8_t msg[3];
  static uint8_t last_val = MIDI_MIN;
  static uint32_t last_update = 0;

  // Guard from values above MIDI_MAX
  val = val % (MIDI_MAX + 1);

  // Clamp outer range to min or max
  if (val >= (MIDI_MAX - CLAMP_RANGE)) {
    val = MIDI_MAX;
  }
  if (val <= (MIDI_MIN + CLAMP_RANGE)) {
    val = MIDI_MIN;
  }

  if (((last_val > val) ? (last_val - val) : (val - last_val)) >= DEVIATION &&
      board_millis() - last_update >= UPDATE_RATE_MS) {
    last_update = board_millis();

    msg[0] = 176; // Control change
    msg[1] = 1;   // Control
    msg[2] = val; // Control val
    tud_midi_n_stream_write(0, 0, msg, 3);

    last_val = val;
  }
}
