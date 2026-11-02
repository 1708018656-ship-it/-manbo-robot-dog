"""
Component placement for the carrier board (100 x 90 mm, 2 layers).

Coordinates: KiCad PCB coordinates in mm, origin top-left, Y down.
Rotation: degrees counter-clockwise as shown in KiCad. Rot 90 makes a 1xN
header run left -> right (pin 1 on the left).

Zones (top to bottom):
  y  0-14  sensors on the front edge: OLED, Bluetooth, 4x IR
  y 18-42  Blue Pill (USB pointing right, towards the board edge)
  y 46-60  servo headers, ASR / SYN6288 / HC-SR04
  y 63-90  power: battery, protection, switch, two buck modules, bulk caps
"""

BOARD_W = 100.0
BOARD_H = 90.0
HOLES = {"H1": (4.0, 4.0), "H2": (96.0, 4.0), "H3": (4.0, 86.0), "H4": (96.0, 86.0)}

# Blue Pill: pin 1 (PB12) at the USB end, top row; rot 270 puts USB on the right.
BP_X, BP_Y = 72.0, 22.0

PLACEMENT = {
    # ---- MCU -----------------------------------------------------------
    "U3": (BP_X, BP_Y, 270),
    "C7": (22.0, 44.5, 0),
    # ---- front edge (top) ----------------------------------------------
    "J9": (22.0, 10.0, 90),      # OLED  GND VCC SCL SDA (left->right)
    "J6": (42.0, 10.0, 90),      # HC-05 EN VCC GND TXD RXD STATE
    "J11": (63.0, 10.0, 90),     # IR front
    "J12": (72.0, 10.0, 90),     # IR back
    "J13": (81.0, 10.0, 90),     # IR left
    "J14": (90.0, 10.0, 90),     # IR right
    # ---- signal row (middle-bottom) ------------------------------------
    "J10": (10.0, 52.0, 90),     # HC-SR04 VCC TRIG ECHO GND
    "R5": (24.0, 48.5, 0),
    "R6": (24.0, 51.5, 0),
    "J7": (30.0, 55.0, 90),      # SYN6288
    "R7": (44.0, 48.0, 0),
    "R8": (44.0, 51.0, 0),
    "C5": (44.0, 54.0, 0),
    "R1": (52.0, 46.5, 90),
    "R2": (55.0, 46.5, 90),
    "R3": (58.0, 46.5, 90),
    "R4": (61.0, 46.5, 90),
    "J2": (52.0, 54.0, 90),      # servo 1  GND V+ SIG
    "J3": (61.0, 54.0, 90),
    "J4": (70.0, 54.0, 90),
    "J5": (79.0, 54.0, 90),
    "J8": (88.0, 48.0, 0),        # ASR PRO (vertical, right edge)
    # ---- power (bottom) --------------------------------------------------
    "J1": (4.0, 66.0, 0),         # battery
    "F1": (10.0, 67.5, 90),
    "Q2": (17.0, 68.5, 0),
    "R12": (17.0, 74.0, 0),
    "Q1": (26.0, 68.5, 0),
    "R9": (26.0, 74.0, 0),
    "R10": (26.0, 77.0, 0),
    "J15": (6.0, 77.5, 0),        # power switch
    "C3": (33.5, 67.0, 90),
    "U2": (40.0, 66.0, 0),        # logic buck   IN at x=40, OUT at x=57.78
    "C4": (60.0, 82.0, 0),
    "C6": (52.0, 81.0, 0),
    "R11": (40.0, 81.0, 0),
    "D1": (45.0, 81.0, 0),
    "U1": (70.0, 66.0, 0),        # servo buck   IN at x=70, OUT at x=87.78
    "C1": (82.0, 81.0, 0),
    "C2": (93.0, 74.0, 90),
}
PLACEMENT.update({k: (x, y, 0) for k, (x, y) in HOLES.items()})
