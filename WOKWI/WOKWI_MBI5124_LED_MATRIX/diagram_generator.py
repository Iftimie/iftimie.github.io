import json

# ===== Matrix size =====
ROWS = 16
COLS = 16
LAYERS = 2  # <-- NEW: two LED layers + two MBI chips

# ===== Placement grid (2x3 cells) =====
CELL_W = 1100
CELL_H = 360

# (0,0) Arduino
UNO_LEFT, UNO_TOP = 40, 40

# (0,1) MBI0, (0,2) MBI1
MBI0_LEFT, MBI0_TOP = UNO_LEFT + 1 * CELL_W, 80
MBI1_LEFT, MBI1_TOP = UNO_LEFT + 2 * CELL_W, 80

# (1,0) 2x 74HC595 (STACKED VERTICALLY)
SR_LEFT = 80
SR_TOP0 = UNO_TOP + CELL_H + 30
SR_GAPY = 95
SR_ROT = 90  # outputs to the RIGHT

# (1,1) LED layer 0 offset, (1,2) LED layer 1 offset
LAYER0_OFFSET_X = UNO_LEFT + 1 * CELL_W
LAYER0_OFFSET_Y = UNO_TOP + 1 * CELL_H

LAYER1_OFFSET_X = UNO_LEFT + 2 * CELL_W
LAYER1_OFFSET_Y = UNO_TOP + 1 * CELL_H

# ===== LED placement style (your preferred layout) =====
START_LEFT = 3.8
START_TOP  = -3.6

COL_SPACING_X = 57.6
ROW_SPACING_Y = 134.4

ANODE_J_TOP_OFFSET  = 75.6
ANODE_J_LEFT_OFFSET = 20.2

COL_J_TOP_OFFSET  = -20.4
COL_J_LEFT_OFFSET = 10.6

ROWEN_LEFT = -110.4
ROWEN_TOP_OFFSET = ANODE_J_TOP_OFFSET
ROWEN_TEXT_LEFT = -96
ROWEN_TEXT_TOP_OFFSET = 51.6

CATHODE_WIRE_ROUTE = ["v-22.8", "h4.2"]

# ===== Pin mapping (UNO) =====
PIN_DATA  = "uno:8"
PIN_CLK   = "uno:9"
PIN_LATCH = "uno:10"
PIN_OE_N  = "uno:11"

# ===== Helpers =====
parts = []
connections = []

def add_part(p):
  parts.append(p)

def add_conn(a, b, color="green", route=None):
  if route is None:
    route = []
  connections.append([a, b, color, route])

def pt_left(x, layer_offset_x):
  return x + layer_offset_x

def pt_top(y, layer_offset_y):
  return y + layer_offset_y

# ===== Add main devices =====
add_part({ "type": "wokwi-arduino-uno", "id": "uno", "top": UNO_TOP, "left": UNO_LEFT, "attrs": {} })

add_part({ "type": "chip-mbi5124", "id": "mbi0", "top": MBI0_TOP, "left": MBI0_LEFT, "attrs": {} })
add_part({ "type": "chip-mbi5124", "id": "mbi1", "top": MBI1_TOP, "left": MBI1_LEFT, "attrs": {} })

add_part({ "type": "wokwi-74hc595", "id": "sr0", "top": SR_TOP0,           "left": SR_LEFT, "rotate": SR_ROT, "attrs": {} })
add_part({ "type": "wokwi-74hc595", "id": "sr1", "top": SR_TOP0 + SR_GAPY, "left": SR_LEFT, "rotate": SR_ROT, "attrs": {} })

# ===== Common-line junctions (bus wiring near Arduino) =====
BUS_LEFT = UNO_LEFT + 250
BUS_TOP  = UNO_TOP + 210
BUS_DY   = 30

add_part({ "type": "wokwi-junction", "id": "J_DATA",  "top": BUS_TOP + 0*BUS_DY, "left": BUS_LEFT, "attrs": {} })
add_part({ "type": "wokwi-junction", "id": "J_CLK",   "top": BUS_TOP + 1*BUS_DY, "left": BUS_LEFT, "attrs": {} })
add_part({ "type": "wokwi-junction", "id": "J_LATCH", "top": BUS_TOP + 2*BUS_DY, "left": BUS_LEFT, "attrs": {} })
add_part({ "type": "wokwi-junction", "id": "J_OE_N",  "top": BUS_TOP + 3*BUS_DY, "left": BUS_LEFT, "attrs": {} })

add_part({ "type": "wokwi-junction", "id": "VCC", "top": UNO_TOP + 120, "left": BUS_LEFT, "attrs": {} })
add_part({ "type": "wokwi-junction", "id": "GND", "top": UNO_TOP + 150, "left": BUS_LEFT, "attrs": {} })

add_part({ "type": "wokwi-text", "id": "T_DATA",  "top": BUS_TOP + 0*BUS_DY - 10, "left": BUS_LEFT + 20, "attrs": {"text":"SR_DATA"} })
add_part({ "type": "wokwi-text", "id": "T_CLK",   "top": BUS_TOP + 1*BUS_DY - 10, "left": BUS_LEFT + 20, "attrs": {"text":"SR_CLK"} })
add_part({ "type": "wokwi-text", "id": "T_LATCH", "top": BUS_TOP + 2*BUS_DY - 10, "left": BUS_LEFT + 20, "attrs": {"text":"SR_LATCH"} })
add_part({ "type": "wokwi-text", "id": "T_OE",    "top": BUS_TOP + 3*BUS_DY - 10, "left": BUS_LEFT + 20, "attrs": {"text":"MBI_OE#"} })
add_part({ "type": "wokwi-text", "id": "T_VCC",   "top": UNO_TOP + 110, "left": BUS_LEFT + 20, "attrs": {"text":"5V"} })
add_part({ "type": "wokwi-text", "id": "T_GND",   "top": UNO_TOP + 140, "left": BUS_LEFT + 20, "attrs": {"text":"GND"} })

# ===== Local power taps for shift-register block =====
PWR_SR_LEFT = SR_LEFT - 90
PWR_SR_TOP  = SR_TOP0 + 10

add_part({ "type": "wokwi-junction", "id": "VCC_SR", "top": PWR_SR_TOP,      "left": PWR_SR_LEFT, "attrs": {} })
add_part({ "type": "wokwi-junction", "id": "GND_SR", "top": PWR_SR_TOP + 24, "left": PWR_SR_LEFT, "attrs": {} })
add_part({ "type": "wokwi-text", "id": "T_VCC_SR", "top": PWR_SR_TOP - 12, "left": PWR_SR_LEFT + 18, "attrs": {"text":"5V"} })
add_part({ "type": "wokwi-text", "id": "T_GND_SR", "top": PWR_SR_TOP + 12, "left": PWR_SR_LEFT + 18, "attrs": {"text":"GND"} })

add_conn("VCC:J", "VCC_SR:J", "red",   ["h-180", "v240", "h-120"])
add_conn("GND:J", "GND_SR:J", "black", ["h-180", "v260", "h-120"])

# ===== Wire UNO to bus =====
add_conn(PIN_DATA,  "J_DATA:J")
add_conn(PIN_CLK,   "J_CLK:J")
add_conn(PIN_LATCH, "J_LATCH:J")
add_conn(PIN_OE_N,  "J_OE_N:J")

add_conn("uno:5V",    "VCC:J", "red")
add_conn("uno:GND.1", "GND:J", "black")

# ===== Wire bus to chips =====
# sr0
add_conn("J_DATA:J",  "sr0:DS")
add_conn("J_CLK:J",   "sr0:SHCP")
add_conn("J_LATCH:J", "sr0:STCP")

# sr1 (clock/latch shared)
add_conn("J_CLK:J",   "sr1:SHCP")
add_conn("J_LATCH:J", "sr1:STCP")

# Daisy chain data: sr0 -> sr1 -> mbi0 -> mbi1
add_conn("sr0:Q7S", "sr1:DS")
add_conn("sr1:Q7S", "mbi0:SDI")

# NOTE: This requires your custom chip to expose SDO (your C model does).
add_conn("mbi0:SDO", "mbi1:SDI")

# MBI clock/latch shared, OE# shared
for mid in ["mbi0", "mbi1"]:
  add_conn("J_CLK:J",   f"{mid}:CLK")
  add_conn("J_LATCH:J", f"{mid}:LE")
  add_conn("J_OE_N:J",  f"{mid}:OE")

# ===== Power wiring =====
# 74HC595: VCC/GND, MR high, OE low (use local taps)
for sid in ["sr0", "sr1"]:
  add_conn("VCC_SR:J", f"{sid}:VCC", "red")
  add_conn("GND_SR:J", f"{sid}:GND", "black")
  add_conn("VCC_SR:J", f"{sid}:MR",  "red")
  add_conn("GND_SR:J", f"{sid}:OE",  "black")

# MBIs: global rails
for mid in ["mbi0", "mbi1"]:
  add_conn("VCC:J", f"{mid}:VDD", "red")
  add_conn("GND:J", f"{mid}:GND", "black")

# ===== Build one LED matrix layer =====
def build_layer(layer_index, layer_offset_x, layer_offset_y, led_prefix):
  led_id = [[None]*COLS for _ in range(ROWS)]
  row_a_id = [[None]*COLS for _ in range(ROWS)]
  rowen_id = [None]*ROWS
  col_k_id = [None]*COLS

  for r in range(ROWS):
    row_top = START_TOP + r * ROW_SPACING_Y

    rowen = f"{led_prefix}_rowen_{r}"
    rowen_id[r] = rowen

    add_part({
      "type": "wokwi-junction",
      "id": rowen,
      "top": pt_top(row_top + ROWEN_TOP_OFFSET, layer_offset_y),
      "left": pt_left(ROWEN_LEFT, layer_offset_x),
      "attrs": {}
    })
    add_part({
      "type": "wokwi-text",
      "id": f"{led_prefix}_text_rowen_{r}",
      "top": pt_top(row_top + ROWEN_TEXT_TOP_OFFSET, layer_offset_y),
      "left": pt_left(ROWEN_TEXT_LEFT, layer_offset_x),
      "attrs": {"text": f"ROWEN_{r}"}
    })

    for c in range(COLS):
      left = START_LEFT + c * COL_SPACING_X
      top  = row_top

      lid = f"{led_prefix}_led_{r*COLS + c}"
      aid = f"{led_prefix}_row{r}_a_{c}"

      led_id[r][c] = lid
      row_a_id[r][c] = aid

      add_part({
        "type": "wokwi-led",
        "id": lid,
        "top": pt_top(top, layer_offset_y),
        "left": pt_left(left, layer_offset_x),
        "attrs": {"color": "red", "lightColor": "black"}
      })

      add_part({
        "type": "wokwi-junction",
        "id": aid,
        "top": pt_top(top + ANODE_J_TOP_OFFSET, layer_offset_y),
        "left": pt_left(left + ANODE_J_LEFT_OFFSET, layer_offset_x),
        "attrs": {}
      })

      add_conn(f"{lid}:A", f"{aid}:J")

      if r == 0:
        ck = f"{led_prefix}_col_{c}_k"
        col_k_id[c] = ck
        add_part({
          "type": "wokwi-junction",
          "id": ck,
          "top": pt_top(top + COL_J_TOP_OFFSET, layer_offset_y),
          "left": pt_left(left + COL_J_LEFT_OFFSET, layer_offset_x),
          "attrs": {}
        })
        add_conn(f"{lid}:C", f"{ck}:J", route=CATHODE_WIRE_ROUTE)
      else:
        add_conn(f"{lid}:C", f"{led_id[r-1][c]}:C", route=["v0"])

    # row rail: rowen -> first tap -> chain
    add_conn(f"{rowen}:J", f"{row_a_id[r][0]}:J", route=["v0"])
    for c in range(COLS - 1):
      add_conn(f"{row_a_id[r][c]}:J", f"{row_a_id[r][c+1]}:J", route=["v0"])

  return rowen_id, col_k_id

# Build both layers
rowen0, colk0 = build_layer(0, LAYER0_OFFSET_X, LAYER0_OFFSET_Y, "L0")
rowen1, colk1 = build_layer(1, LAYER1_OFFSET_X, LAYER1_OFFSET_Y, "L1")

# ===== Connect ROWEN signals to BOTH layers (same enable lines) =====
# sr0 Q0..Q7 -> ROWEN_0..7
for r in range(8):
  add_conn(f"sr0:Q{r}", f"{rowen0[r]}:J")
  add_conn(f"sr0:Q{r}", f"{rowen1[r]}:J")

# sr1 Q0..Q7 -> ROWEN_8..15
for r in range(8, 16):
  add_conn(f"sr1:Q{r-8}", f"{rowen0[r]}:J")
  add_conn(f"sr1:Q{r-8}", f"{rowen1[r]}:J")

# ===== Connect MBI outputs to columns (each MBI drives its own layer columns) =====
for c in range(COLS):
  add_conn(f"mbi0:OUT{c}", f"{colk0[c]}:J")
  add_conn(f"mbi1:OUT{c}", f"{colk1[c]}:J")

# ===== Emit diagram =====
diagram = {
  "version": 1,
  "author": "generator",
  "editor": "wokwi",
  "parts": parts,
  "connections": connections,
  "dependencies": {}
}

with open("diagram.json", "w", encoding="utf-8") as f:
  json.dump(diagram, f, indent=2)

print("Generated diagram.json: 2x3 grid with two LED layers + two MBIs, shared ROWEN enables, and full daisy-chained data.")
