// Wokwi Custom Chip - MBI5124 (virtual)
// SPDX-License-Identifier: MIT

#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc

typedef struct {
  pin_t sdi, clk, le, oe, sdo;
  pin_t out[16];

  uint16_t shift_reg;
  uint16_t latch_reg;

  bool last_clk;
  bool last_le;
} chip_state_t;

static void update_outputs(chip_state_t *s) {
  // Model OE as OE# (active LOW): LOW enables outputs, HIGH blanks
  bool enabled = pin_read(s->oe) != 0;

  for (int i = 0; i < 16; i++) {
    bool bit_on = ((s->latch_reg >> i) & 1) != 0;
    bool sink_on = enabled && bit_on;

    // Sink output behavior: LOW = ON (sinking), HIGH = OFF
    pin_write(s->out[i], sink_on ? LOW : HIGH);
  }
}

static void on_pin_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *s = (chip_state_t *)user_data;

  if (pin == s->clk) {
    bool clk = value != 0;

    // Rising edge
    if (!s->last_clk && clk) {
      bool din = (pin_read(s->sdi) != 0);

      // MSB-first shift: shift left, DIN into bit0, bit15 shifts out
      uint32_t out_bit = (s->shift_reg >> 15) & 1;
      s->shift_reg = (uint16_t)((s->shift_reg << 1) | (din ? 1 : 0));

      // Daisy-chain output
      pin_write(s->sdo, out_bit ? HIGH : LOW);
    }
    s->last_clk = clk;
  }

  if (pin == s->le) {
    bool le = value != 0;

    // Rising edge latch
    if (!s->last_le && le) {
      s->latch_reg = s->shift_reg;
      update_outputs(s);
    }
    s->last_le = le;
  }

  // OE changes affect outputs immediately
  if (pin == s->oe) {
    update_outputs(s);
  }
}

void chip_init(void) {
  chip_state_t *s = (chip_state_t *)calloc(1, sizeof(chip_state_t));

  s->sdi = pin_init("SDI", INPUT);
  s->clk = pin_init("CLK", INPUT);
  s->le  = pin_init("LE",  INPUT);
  s->oe  = pin_init("OE",  INPUT);
  s->sdo = pin_init("SDO", OUTPUT);

  for (int i = 0; i < 16; i++) {
    char name[8];
    snprintf(name, sizeof(name), "OUT%d", i);
    s->out[i] = pin_init(name, OUTPUT_HIGH); // default OFF (HIGH)
  }

  // Watch pin edges
  const pin_watch_config_t watch_clk = {
    .edge = BOTH,
    .pin_change = on_pin_change,
    .user_data = s,
  };
  pin_watch(s->clk, &watch_clk);

  const pin_watch_config_t watch_le = {
    .edge = BOTH,
    .pin_change = on_pin_change,
    .user_data = s,
  };
  pin_watch(s->le, &watch_le);

  const pin_watch_config_t watch_oe = {
    .edge = BOTH,
    .pin_change = on_pin_change,
    .user_data = s,
  };
  pin_watch(s->oe, &watch_oe);

  // Initialize outputs
  update_outputs(s);
}
