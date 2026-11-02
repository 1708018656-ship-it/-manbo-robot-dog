"""
Footprint geometry for the PCB generator (embedded in the .kicad_pcb, so the
board opens without any extra library). Dimensions follow the KiCad 8
standard-library footprints of the same name (IPC nominal); custom "manbo:"
footprints are defined here.

All coordinates in mm, footprint origin = pin 1 for connectors (as in KiCad),
centre for SMD parts.
"""

from dataclasses import dataclass, field
from typing import List, Optional, Tuple


@dataclass
class Pad:
    num: str
    kind: str          # "thru_hole" | "smd" | "np_thru_hole"
    shape: str         # "circle" | "rect" | "roundrect" | "oval"
    x: float
    y: float
    w: float
    h: float
    drill: float = 0.0


@dataclass
class Footprint:
    name: str
    pads: List[Pad]
    body: Tuple[float, float, float, float]          # x1 y1 x2 y2 (silk/courtyard box)
    silk_lines: List[Tuple[float, float, float, float]] = field(default_factory=list)
    smd: bool = False


def _header(n, pitch=2.54, drill=1.0, pad=1.7):
    pads = []
    for i in range(n):
        pads.append(Pad(str(i + 1), "thru_hole", "rect" if i == 0 else "oval",
                        0.0, i * pitch, pad, pad, drill))
    return pads


def get(name):
    lib, fp = name.split(":", 1)

    if fp.startswith("PinHeader_1x"):
        n = int(fp[len("PinHeader_1x"):len("PinHeader_1x") + 2])
        return Footprint(name, _header(n), (-1.27, -1.27, 1.27, (n - 1) * 2.54 + 1.27))

    if fp == "JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical":
        pads = [Pad("1", "thru_hole", "roundrect", 0, 0, 1.7, 2.0, 0.95),
                Pad("2", "thru_hole", "oval", 0, 2.5, 1.7, 2.0, 0.95)]
        # body 5.75 x 7.45 (rotated so pins run along +Y)
        return Footprint(name, pads, (-2.45, -2.45, 3.4, 4.95))

    if fp == "SOIC-8_3.9x4.9mm_P1.27mm":
        pads = []
        for i in range(4):
            pads.append(Pad(str(i + 1), "smd", "roundrect", -2.475, -1.905 + i * 1.27, 1.95, 0.6))
        for i in range(4):
            pads.append(Pad(str(8 - i), "smd", "roundrect", 2.475, -1.905 + i * 1.27, 1.95, 0.6))
        return Footprint(name, pads, (-3.7, -2.7, 3.7, 2.7), smd=True)

    if fp in ("R_0805_2012Metric", "C_0805_2012Metric", "LED_0805_2012Metric"):
        pads = [Pad("1", "smd", "roundrect", -0.9125, 0, 1.025, 1.4),
                Pad("2", "smd", "roundrect", 0.9125, 0, 1.025, 1.4)]
        return Footprint(name, pads, (-1.7, -1.0, 1.7, 1.0), smd=True)

    if fp == "Fuse_1812_4532Metric":
        pads = [Pad("1", "smd", "roundrect", -2.1375, 0, 1.325, 3.4),
                Pad("2", "smd", "roundrect", 2.1375, 0, 1.325, 3.4)]
        return Footprint(name, pads, (-3.1, -2.0, 3.1, 2.0), smd=True)

    if fp.startswith("CP_Radial_D"):
        d = float(fp.split("_D")[1].split("mm")[0])
        p = float(fp.split("_P")[1].split("mm")[0])
        pad = 2.0 if d >= 8 else 1.6
        drill = 1.0 if d >= 8 else 0.8
        pads = [Pad("1", "thru_hole", "rect", 0, 0, pad, pad, drill),
                Pad("2", "thru_hole", "circle", p, 0, pad, pad, drill)]
        r = d / 2 + 0.25
        return Footprint(name, pads, (p / 2 - r, -r, p / 2 + r, r))

    if fp == "MountingHole_3.2mm_M3":
        return Footprint(name, [Pad("", "np_thru_hole", "circle", 0, 0, 3.2, 3.2, 3.2)],
                         (-3.2, -3.2, 3.2, 3.2))

    if fp == "BluePill_Socket_2x20_W15.24mm":
        # Pin 1..20 = left row (x=0), 21..40 = right row (x=15.24), both from the USB end (y=0).
        pads = []
        for i in range(20):
            pads.append(Pad(str(i + 1), "thru_hole", "rect" if i == 0 else "oval", 0, i * 2.54, 1.7, 1.7, 1.0))
        for i in range(20):
            pads.append(Pad(str(21 + i), "thru_hole", "oval", 15.24, i * 2.54, 1.7, 1.7, 1.0))
        # Blue Pill PCB is 22.9 x 53.0 mm; USB end at y < 0
        return Footprint(name, pads, (-3.83, -2.5, 19.07, 50.5),
                         silk_lines=[(-3.83, -2.5, 19.07, -2.5)])

    if fp == "BuckModule_2x2_P2.54mm_17.78mm":
        # IN+ IN- on the left, OUT+ OUT- on the right; 1.2 mm holes accept
        # module pins or 20-22 AWG wires.
        pads = [Pad("1", "thru_hole", "rect", 0, 0, 2.2, 2.2, 1.2),
                Pad("2", "thru_hole", "circle", 0, 2.54, 2.2, 2.2, 1.2),
                Pad("3", "thru_hole", "rect", 17.78, 0, 2.2, 2.2, 1.2),
                Pad("4", "thru_hole", "circle", 17.78, 2.54, 2.2, 2.2, 1.2)]
        return Footprint(name, pads, (-2.2, -7.2, 19.98, 9.74))

    raise KeyError("no footprint geometry for " + name)
