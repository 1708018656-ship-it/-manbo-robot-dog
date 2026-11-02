"""Placement for the LITE board (70 x 54 mm, 2 layers). See layout.py for conventions."""

BOARD_W = 70.0
BOARD_H = 54.0
BRAND = ("MANBO LITE L1.0", 14.0, 52.0)
HOLES = {"H1": (4.0, 4.0), "H2": (66.0, 4.0), "H3": (4.0, 50.0), "H4": (66.0, 50.0)}

PLACEMENT = {
    "U1": (62.0, 12.0, 270),      # Blue Pill, USB to the right edge
    "J1": (4.0, 16.0, 0),         # battery box
    "D1": (8.0, 35.5, 0),
    "D2": (17.0, 35.5, 0),
    "C1": (25.0, 37.5, 0),
    "C2": (37.0, 37.0, 0),
    "C3": (48.0, 39.5, 0),
    "J2": (14.0, 46.0, 90),       # servo 1  GND V+ SIG
    "J3": (22.5, 46.0, 90),
    "J4": (31.0, 46.0, 90),
    "J5": (39.5, 46.0, 90),
    "J6": (52.0, 46.0, 90),       # SU-03T  VCC GND TX RX
    "J7": (24.0, 4.0, 90),        # OLED    GND VCC SCL SDA (top edge)
}
PLACEMENT.update({k: (x, y, 0) for k, (x, y) in HOLES.items()})
