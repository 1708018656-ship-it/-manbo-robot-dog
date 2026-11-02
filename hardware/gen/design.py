"""
Manbo carrier board V2 — single source of truth for parts and nets.

Everything under hardware/kicad/ (schematic, PCB, BOM, pin map) is generated
from this file by build.py. Edit here, then run:  python3 hardware/gen/build.py

Signal pins follow User/BoardConfig.h with MANBO_BOARD_V2 defined.
"""

from dataclasses import dataclass, field
from typing import Dict, List, Tuple

# ---------------------------------------------------------------------------
# Blue Pill (STM32F103C8T6 minimum-system board) pin order, top view, USB up.
# Left row is pin 1..20 from the USB end, right row 21..40 from the USB end.
# Verify against your own board: a few clones differ in silkscreen only.
# ---------------------------------------------------------------------------
BLUEPILL_LEFT = ["PB12", "PB13", "PB14", "PB15", "PA8", "PA9", "PA10", "PA11",
                 "PA12", "PA15", "PB3", "PB4", "PB5", "PB6", "PB7", "PB8",
                 "PB9", "5V", "GND", "3V3"]
BLUEPILL_RIGHT = ["GND", "GND", "3V3", "NRST", "PB11", "PB10", "PB1", "PB0",
                  "PA7", "PA6", "PA5", "PA4", "PA3", "PA2", "PA1", "PA0",
                  "PC15", "PC14", "PC13", "VBAT"]


@dataclass
class Part:
    ref: str
    value: str
    footprint: str            # KiCad library footprint name (standard libs where possible)
    pins: List[Tuple[str, str]]  # (number, name) in symbol order
    nets: Dict[str, str]      # pin number -> net name ; missing = no connect
    desc: str = ""
    lcsc: str = ""            # LCSC / Taobao search keyword (not a catalogue number)
    dnp: bool = False
    group: str = ""

    def net_of(self, num):
        return self.nets.get(num)


def two_pin(ref, value, fp, n1, n2, desc="", lcsc="", group=""):
    return Part(ref, value, fp, [("1", "1"), ("2", "2")], {"1": n1, "2": n2},
                desc, lcsc, group=group)


def header(ref, value, names, nets, desc="", fp=None, group=""):
    n = len(names)
    pins = [(str(i + 1), names[i]) for i in range(n)]
    netmap = {str(i + 1): nets[i] for i in range(n) if nets[i]}
    if fp is None:
        fp = "Connector_PinHeader_2.54mm:PinHeader_1x%02d_P2.54mm_Vertical" % n
        kw = "2.54mm header %dP" % n
    else:
        kw = "XH2.54 %dP" % n if "JST_XH" in fp else ""
    return Part(ref, value, fp, pins, netmap, desc, kw, group=group)


R0805 = "Resistor_SMD:R_0805_2012Metric"
C0805 = "Capacitor_SMD:C_0805_2012Metric"

PARTS: List[Part] = []
P = PARTS.append

# ---------------------------------------------------------------- power ----
P(Part("J1", "BAT 2S 7.4V", "Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical",
       [("1", "BAT+"), ("2", "BAT-")], {"1": "VBAT_IN", "2": "GND"},
       "2S Li-ion/LiPo input (6.4-8.4 V)", "XH2.54 2P", group="power"))
# Worst case: 4x SG90 stall 2.8 A @ 5 V + logic 0.3 A -> ~2.4 A from a 7.4 V pack.
# 1812-size PTCs top out around 2.6-3 A and are often rated only 6-8 V (< 8.4 V
# full 2S), so use a 2920 part rated >= 16 V.
P(two_pin("F1", "PTC 3A 16V", "Fuse:Fuse_2920_7451Metric", "VBAT_IN", "VBAT_F",
          "Resettable fuse, 3 A hold, >=16 V (servo stall protection)", "SMD2920P300TF/24", "power"))
# Q2: reverse-polarity protection. P-MOSFET with Drain on the battery side:
#     body diode conducts first, then Vgs = -VBAT turns the channel on.
#     Battery reversed -> body diode blocks and Vgs > 0 keeps it off.
P(Part("Q2", "AO4407A", "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm",
       [("1", "S"), ("2", "S"), ("3", "S"), ("4", "G"),
        ("5", "D"), ("6", "D"), ("7", "D"), ("8", "D")],
       {"1": "VBAT_RP", "2": "VBAT_RP", "3": "VBAT_RP", "4": "Q2_GATE",
        "5": "VBAT_F", "6": "VBAT_F", "7": "VBAT_F", "8": "VBAT_F"},
       "P-MOSFET -30V -12A, reverse-polarity protection", "AO4407A", group="power"))
P(two_pin("R12", "10k", R0805, "Q2_GATE", "GND", "Q2 gate to GND", "0805 10k", "power"))
# Q1: high-side power switch. Source on the supply side so the body diode
#     cannot leak current to the load while off (a single FET cannot be both
#     the switch and the reverse protection). A small panel switch pulls the
#     gate low; the full servo current never flows through the switch.
P(Part("Q1", "AO4407A", "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm",
       [("1", "S"), ("2", "S"), ("3", "S"), ("4", "G"),
        ("5", "D"), ("6", "D"), ("7", "D"), ("8", "D")],
       {"1": "VBAT_RP", "2": "VBAT_RP", "3": "VBAT_RP", "4": "Q1_GATE",
        "5": "VBAT", "6": "VBAT", "7": "VBAT", "8": "VBAT"},
       "P-MOSFET -30V -12A, power switch", "AO4407A", group="power"))
P(two_pin("R9", "100k 1%", R0805, "VBAT_RP", "Q1_GATE", "Gate pull-up: off when switch open", "0805 100k 1%", "power"))
P(two_pin("R10", "1k", R0805, "Q1_GATE", "SW_GATE", "Gate series resistor", "0805 1k", "power"))
P(header("J15", "POWER SW", ["SW", "GND"], ["SW_GATE", "GND"],
         "External ON/OFF switch (signal level, any small switch)",
         fp="Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical", group="power"))
P(two_pin("C3", "100uF 16V", "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm", "VBAT", "GND",
          "VBAT bulk", "100uF 16V 6.3x7", "power"))

# Two identical buck-module positions: servo rail and logic rail kept apart so
# a servo stall can no longer brown-out the MCU (the original single-rail
# build reset when all four SG90s started together).
for ref, out, use in (("U1", "+5V_SERVO", "servo rail, >=3 A (e.g. Mini560 5V / MP1584)"),
                      ("U2", "+5V", "logic rail, >=1 A (e.g. MP1584 / Mini360 set to 5.0 V)")):
    P(Part(ref, "BUCK 5V", "manbo:BuckModule_2x2_P2.54mm_17.78mm",
           [("1", "IN+"), ("2", "IN-"), ("3", "OUT+"), ("4", "OUT-")],
           {"1": "VBAT", "2": "GND", "3": out, "4": "GND"},
           "DC-DC buck module, " + use, "MP1584 / Mini560 module", group="power"))
P(two_pin("C1", "1000uF 10V", "Capacitor_THT:CP_Radial_D10.0mm_P5.00mm", "+5V_SERVO", "GND",
          "Servo rail bulk (absorbs SG90 start-up current)", "1000uF 10V 10x13", "power"))
P(two_pin("C2", "100nF", C0805, "+5V_SERVO", "GND", "Servo rail HF decoupling", "0805 100nF", "power"))
P(two_pin("C4", "220uF 10V", "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm", "+5V", "GND",
          "Logic rail bulk", "220uF 10V 6.3x7", "power"))
P(two_pin("C6", "100nF", C0805, "+5V", "GND", "Logic rail HF decoupling", "0805 100nF", "power"))
# ------------------------------------------------------------------ MCU ----
BP_NETS = {
    "PB12": "IR_FRONT", "PB13": "IR_BACK", "PB14": "IR_LEFT", "PB15": "IR_RIGHT",
    "PA9": "BT_RXD", "PA10": "BT_TXD",             # USART1 (names from module side)
    "PB8": "OLED_SCL", "PB9": "OLED_SDA",
    "PB10": "ASR_RXD", "PB11": "ASR_TXD",          # USART3
    "PB0": "SERVO3_PWM", "PB1": "SERVO4_PWM",
    "PA6": "SERVO1_PWM", "PA7": "SERVO2_PWM",      # TIM3 CH1..CH4
    "PA4": "VBAT_SENSE",                           # ADC1_IN4
    "PA2": "SYN_RXD", "PA3": "SYN_TXD",            # USART2
    "PA0": "US_TRIG", "PA1": "US_ECHO_3V3",
    "5V": "+5V", "GND": "GND", "3V3": "+3V3",
}
bp_pins, bp_nets = [], {}
for i, name in enumerate(BLUEPILL_LEFT + BLUEPILL_RIGHT):
    num = str(i + 1)
    bp_pins.append((num, name))
    if name in BP_NETS:
        bp_nets[num] = BP_NETS[name]
P(Part("U3", "BluePill STM32F103C8T6", "manbo:BluePill_Socket_2x20_W15.24mm",
       bp_pins, bp_nets, "Blue Pill on 2x 1x20 female headers", "Blue Pill STM32F103C8T6",
       group="mcu"))

# ---------------------------------------------------------------- servos ---
# Signals go straight to the headers (no series resistors: an SG90 signal
# wire shorting to V+ is rare, and the parts only added solder joints).
for n in range(1, 5):
    P(header("J%d" % (n + 1), "SERVO%d" % n, ["GND", "V+", "SIG"],
             ["GND", "+5V_SERVO", "SERVO%d_PWM" % n],
             "SG90 servo %d (brown=GND, red=V+, orange=SIG)" % n, group="servo"))

# --------------------------------------------------------------- modules ---
P(header("J6", "HC-05/06", ["EN", "VCC", "GND", "TXD", "RXD", "STATE"],
         [None, "+5V", "GND", "BT_TXD", "BT_RXD", None],
         "Bluetooth module socket (HC-05 6-pin; HC-06 4-pin plugs into pins 2-5)",
         group="modules"))
P(header("J7", "SYN6288", ["VCC", "GND", "RXD", "TXD"],
         ["+5V", "GND", "SYN_RXD", "SYN_TXD"], "SYN6288 speech synthesis module", group="modules"))
P(header("J8", "ASR PRO", ["VCC", "GND", "RXD", "TXD"],
         ["+5V", "GND", "ASR_RXD", "ASR_TXD"], "ASR PRO voice recognition module", group="modules"))
P(header("J9", "OLED I2C", ["GND", "VCC", "SCL", "SDA"],
         ["GND", "+3V3", "OLED_SCL", "OLED_SDA"], "0.96in SSD1306 I2C OLED (plugs in directly)",
         group="modules"))
P(header("J10", "HC-SR04", ["VCC", "TRIG", "ECHO", "GND"],
         ["+5V", "US_TRIG", "US_ECHO_5V", "GND"], "Ultrasonic sensor (plugs in directly)",
         group="modules"))
# Echo is a 5 V output; PA1 is NOT 5 V tolerant -> divide to 3.33 V.
P(two_pin("R5", "1k", R0805, "US_ECHO_5V", "US_ECHO_3V3", "Echo level shift (top)", "0805 1k", "modules"))
P(two_pin("R6", "2k", R0805, "US_ECHO_3V3", "GND", "Echo level shift (bottom)", "0805 2k", "modules"))
for i, (ref, side) in enumerate((("J11", "FRONT"), ("J12", "BACK"), ("J13", "LEFT"), ("J14", "RIGHT"))):
    P(header(ref, "IR " + side, ["VCC", "GND", "OUT"], ["+5V", "GND", "IR_" + side],
             "IR cliff sensor %s (FC-51 style, VCC GND OUT)" % side.lower(), group="modules"))

# ------------------------------------------------------- battery sensing ---
P(two_pin("R7", "100k 1%", R0805, "VBAT", "VBAT_SENSE", "VBAT divider top", "0805 100k 1%", "sense"))
P(two_pin("R8", "33k 1%", R0805, "VBAT_SENSE", "GND", "VBAT divider bottom", "0805 33k 1%", "sense"))
P(two_pin("C5", "100nF", C0805, "VBAT_SENSE", "GND", "ADC input filter", "0805 100nF", "sense"))

# --------------------------------------------------------------- mechanics -
for i in range(1, 5):
    P(Part("H%d" % i, "M3", "MountingHole:MountingHole_3.2mm_M3", [], {}, "M3 mounting hole",
           group="mech"))

POWER_NETS = {"GND", "VBAT_IN", "VBAT_F", "VBAT_RP", "VBAT", "+5V_SERVO", "+5V", "+3V3"}
# Track widths (mm) used by the PCB generator
NET_WIDTH = {"GND": 1.0, "VBAT_IN": 1.5, "VBAT_F": 1.5, "VBAT_RP": 1.5, "VBAT": 1.5,
             "+5V_SERVO": 1.5, "+5V": 0.6, "+3V3": 0.4}
DEFAULT_WIDTH = 0.25


def nets():
    out = {}
    for p in PARTS:
        for num, net in p.nets.items():
            out.setdefault(net, []).append((p.ref, num))
    return out


# firmware fixed pins that the board must match (checked by build.py)
FIRMWARE_PINS = {"PA6": "SERVO1_PWM", "PA7": "SERVO2_PWM", "PB0": "SERVO3_PWM", "PB1": "SERVO4_PWM",
                 "PA9": "BT_RXD", "PA10": "BT_TXD", "PA2": "SYN_RXD", "PA3": "SYN_TXD",
                 "PB10": "ASR_RXD", "PB11": "ASR_TXD", "PB8": "OLED_SCL", "PB9": "OLED_SDA",
                 "PA0": "US_TRIG", "PA1": "US_ECHO_3V3"}
