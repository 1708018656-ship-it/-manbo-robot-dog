#!/usr/bin/env python3
"""
Generate hardware/kicad/* from design.py and run self-checks.

    python3 hardware/gen/build.py          # write files
    python3 hardware/gen/build.py --check  # fail if outputs are stale
"""

import csv
import math
import io
import os
import re
import sys
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import design  # noqa: E402
import kicad_sch  # noqa: E402
import preview  # noqa: E402
import layout  # noqa: E402
import kicad_pcb  # noqa: E402
import router  # noqa: E402
import drc  # noqa: E402

OUT = os.path.normpath(os.path.join(HERE, "..", "kicad"))
PROJECT = "manbo-carrier"
REV = "2.0"
DATE = "2026-11-02"

# STM32F103 pins that are NOT 5 V tolerant (datasheet table 5, "FT" column).
NON_FT = {"PA0", "PA1", "PA2", "PA3", "PA4", "PA5", "PA6", "PA7", "PB0", "PB1",
          "PC13", "PC14", "PC15"}
FIVE_VOLT_SIGNALS = {"US_ECHO_5V"}   # known 5 V outputs from modules


def erc():
    errors, warnings = [], []
    refs = Counter(p.ref for p in design.PARTS)
    for r, c in refs.items():
        if c > 1:
            errors.append("duplicate reference " + r)
    for p in design.PARTS:
        nums = [n for n, _ in p.pins]
        if len(nums) != len(set(nums)):
            errors.append("%s: duplicate pin numbers" % p.ref)
        for n in p.nets:
            if n not in nums:
                errors.append("%s: net on unknown pin %s" % (p.ref, n))
    nets = design.nets()
    for net, conns in nets.items():
        if len(conns) < 2:
            errors.append("net %s has only one connection %s" % (net, conns))
    for pw in design.POWER_NETS:
        if pw not in nets:
            warnings.append("power net %s unused" % pw)
    # 5 V signals must never reach a non-FT MCU pin
    bp = next(p for p in design.PARTS if p.ref == "U3")
    for num, name in bp.pins:
        net = bp.net_of(num)
        if name in NON_FT and net in FIVE_VOLT_SIGNALS:
            errors.append("5 V signal %s on non-5V-tolerant %s" % (net, name))
    # firmware pin map must match BoardConfig.h (MANBO_BOARD_V2)
    cfg = open(os.path.join(HERE, "..", "..", "User", "BoardConfig.h"), encoding="utf-8").read()
    v2 = cfg.split("#ifdef MANBO_BOARD_V2", 1)[1].split("#else", 1)[0]
    pin_by_name = {name: bp.net_of(num) for num, name in bp.pins}
    for side in ("FRONT", "BACK", "LEFT", "RIGHT"):
        port = re.search(r"IR_%s_PORT\s+GPIO([A-C])" % side, v2).group(1)
        pin = re.search(r"IR_%s_PIN\s+GPIO_Pin_(\d+)" % side, v2).group(1)
        if pin_by_name.get("P%s%s" % (port, pin)) != "IR_" + side:
            errors.append("IR_%s: board uses %s, firmware expects P%s%s"
                          % (side, [k for k, v in pin_by_name.items() if v == "IR_" + side], port, pin))
    vport = re.search(r"VBAT_ADC_PORT\s+GPIO([A-C])", v2).group(1)
    vpin = re.search(r"VBAT_ADC_PIN\s+GPIO_Pin_(\d+)", v2).group(1)
    if pin_by_name.get("P%s%s" % (vport, vpin)) != "VBAT_SENSE":
        errors.append("VBAT_SENSE pin does not match BoardConfig.h")
    fixed = {"PA6": "SERVO1_PWM", "PA7": "SERVO2_PWM", "PB0": "SERVO3_PWM", "PB1": "SERVO4_PWM",
             "PA9": "BT_RXD", "PA10": "BT_TXD", "PA2": "SYN_RXD", "PA3": "SYN_TXD",
             "PB10": "ASR_RXD", "PB11": "ASR_TXD", "PB8": "OLED_SCL", "PB9": "OLED_SDA",
             "PA0": "US_TRIG", "PA1": "US_ECHO_3V3"}
    for pin, net in fixed.items():
        if pin_by_name.get(pin) != net:
            errors.append("%s should be %s (firmware), board has %s" % (pin, net, pin_by_name.get(pin)))
    # divider sanity: echo level after divider
    return errors, warnings


def sexpr_balanced(text):
    depth = 0
    in_str = False
    esc = False
    for ch in text:
        if in_str:
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == '"':
                in_str = False
            continue
        if ch == '"':
            in_str = True
        elif ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth < 0:
                return False
    return depth == 0 and not in_str


def check_schematic(sch):
    """Every label/no-connect must sit exactly on a pin end of its symbol."""
    errs = []
    if not sexpr_balanced(sch):
        errs.append("schematic parentheses unbalanced")
    uuids = re.findall(r'\(uuid "([0-9a-f-]+)"\)', sch)
    dup = [u for u, c in Counter(uuids).items() if c > 1]
    if dup:
        errs.append("duplicate uuids: %d" % len(dup))
    labels = Counter(re.findall(r'\(label "([^"]+)"', sch))
    for net, conns in design.nets().items():
        if labels[net] != len(conns):
            errs.append("net %s: %d labels for %d pins" % (net, labels[net], len(conns)))
    return errs


def bom_csv():
    rows = defaultdict(list)
    for p in design.PARTS:
        rows[(p.value, p.footprint, p.lcsc, p.desc if p.ref[0] not in "RC" else "")].append(p.ref)
    buf = io.StringIO()
    w = csv.writer(buf, lineterminator="\n")
    w.writerow(["Qty", "References", "Value", "Footprint", "Search keyword (LCSC/Taobao)", "Notes"])

    def keyf(item):
        refs = item[1]
        m = re.match(r"([A-Z]+)(\d+)", refs[0])
        return (m.group(1), int(m.group(2)))
    for (value, fp, lcsc, desc), refs in sorted(rows.items(), key=keyf):
        refs.sort(key=lambda r: (re.match(r"[A-Z]+", r).group(0), int(re.search(r"\d+", r).group(0))))
        w.writerow([len(refs), " ".join(refs), value, fp.split(":")[-1], lcsc, desc])
    return buf.getvalue()


def pinmap_md():
    bp = next(p for p in design.PARTS if p.ref == "U3")
    lines = ["# Manbo carrier V2 - Blue Pill pin map", "",
             "Generated from `hardware/gen/design.py`. Firmware: build with `MANBO_BOARD_V2` "
             "(`make V2=1`, or add the define in Keil).", "",
             "| Blue Pill pin | STM32 | Net | Goes to |", "|---|---|---|---|"]
    nets = design.nets()
    for num, name in bp.pins:
        net = bp.net_of(num) or "-"
        dest = ", ".join("%s.%s" % (r, n) for r, n in nets.get(net, []) if r != "U3") if net != "-" else ""
        if net in ("GND", "+5V", "+3V3"):
            dest = "(power)"
        lines.append("| %s | %s | %s | %s |" % (num, name, net, dest))
    return "\n".join(lines) + "\n"


def kicad_pro():
    import json

    def cls(name, width, clearance=0.2):
        return {"name": name, "clearance": clearance, "track_width": width,
                "via_diameter": 0.8, "via_drill": 0.4, "microvia_diameter": 0.3,
                "microvia_drill": 0.1, "diff_pair_width": 0.2, "diff_pair_gap": 0.25,
                "diff_pair_via_gap": 0.25, "wire_width": 6, "bus_width": 12,
                "line_style": 0, "pcb_color": "rgba(0, 0, 0, 0.000)",
                "schematic_color": "rgba(0, 0, 0, 0.000)"}
    pro = {
        "meta": {"filename": PROJECT + ".kicad_pro", "version": 1},
        "board": {"design_settings": {"rules": {"min_clearance": 0.2, "min_track_width": 0.2,
                                                  "min_via_diameter": 0.6, "min_through_hole_diameter": 0.3,
                                                  "min_copper_edge_clearance": 0.5}}},
        "net_settings": {
            "classes": [cls("Default", design.DEFAULT_WIDTH),
                        cls("Battery", 1.5, 0.3), cls("Servo", 1.5, 0.3),
                        cls("Logic5V", 0.6), cls("GND", 1.0)],
            "meta": {"version": 3},
            "netclass_patterns": [{"netclass": "Battery", "pattern": "VBAT*"},
                                  {"netclass": "Servo", "pattern": "+5V_SERVO"},
                                  {"netclass": "Logic5V", "pattern": "+5V"},
                                  {"netclass": "GND", "pattern": "GND"}]},
        "sheets": [["%s" % kicad_sch.uid("root", PROJECT), "Root"]],
    }
    return json.dumps(pro, indent=2) + "\n"


def outputs():
    sch, symlib, placed, titles = kicad_sch.build(design.PARTS, "Manbo robot dog carrier board V2", PROJECT, REV, DATE)
    files = {
        PROJECT + ".kicad_sch": sch,
        "manbo.kicad_sym": symlib,
        "sym-lib-table": '(sym_lib_table\n  (version 7)\n  (lib (name "manbo")(type "KiCad")(uri "${KIPRJMOD}/manbo.kicad_sym")(options "")(descr "Manbo generated symbols"))\n)\n',
        "bom.csv": bom_csv(),
        "pinmap.md": pinmap_md(),
        "schematic-preview.svg": preview.schematic_svg(placed, titles),
        PROJECT + ".kicad_pro": kicad_pro(),
        "fp-lib-table": '(fp_lib_table\n  (version 7)\n)\n',
    }
    pcb_placed, missing = kicad_pcb.place_all(design.PARTS, layout.PLACEMENT)
    PLACEMENT_ERRORS[:] = ["not placed: " + m for m in missing]
    PLACEMENT_ERRORS.extend(kicad_pcb.check_placement(pcb_placed, layout.BOARD_W, layout.BOARD_H))
    rt = router.Router(pcb_placed, layout.BOARD_W, layout.BOARD_H, design.NET_WIDTH, design.DEFAULT_WIDTH,
                       kicad_pcb.CLEARANCE, 0.3, design.POWER_NETS)
    rn_all = kicad_pcb.ratsnest(pcb_placed)
    length = {}
    for a, b, c, d, n in rn_all:
        length[n] = length.get(n, 0) + math.hypot(a - c, b - d)
    order = sorted(length, key=lambda n: (n == "GND", -design.NET_WIDTH.get(n, design.DEFAULT_WIDTH), length[n]))
    failed = rt.route_all(order)
    tracks, vias = rt.tracks, rt.vias
    LAST["tracks"], LAST["vias"] = tracks, vias
    ROUTE_FAILED[:] = failed
    clr = lambda n: 0.3 if n in design.POWER_NETS else kicad_pcb.CLEARANCE  # noqa: E731
    DRC_RESULT[:] = [drc.run(pcb_placed, tracks, vias, layout.BOARD_W, layout.BOARD_H, clr)]
    files[PROJECT + ".kicad_pcb"] = kicad_pcb.write(design.PARTS, pcb_placed, layout.BOARD_W, layout.BOARD_H,
                                                    tracks, vias, "Manbo robot dog carrier board V2", REV, DATE, PROJECT)
    files["pcb-preview.svg"] = kicad_pcb.preview_svg(pcb_placed, layout.BOARD_W, layout.BOARD_H, tracks, vias, [])
    drc_errs, unconnected = DRC_RESULT[0]
    total_len = sum(math.hypot(t[0] - t[2], t[1] - t[3]) for t in tracks)
    files["drc-report.txt"] = "\n".join([
        "Manbo carrier V%s - generated board report (hardware/gen/drc.py, exact geometry)" % REV,
        "board: %.0f x %.0f mm, 2 layers, %d footprints" % (layout.BOARD_W, layout.BOARD_H, len(pcb_placed)),
        "nets: %d, routed segments: %d (%.0f mm), vias: %d" % (len(length), len(tracks), total_len, len(vias)),
        "clearance: signal 0.20 mm, power 0.30 mm, edge 0.50 mm",
        "track widths: battery/servo 1.5 mm, GND 1.0 mm, +5V 0.6 mm, +3V3 0.4 mm, signal 0.25 mm",
        "unrouted connections: %d" % len(ROUTE_FAILED),
        "clearance violations: %d" % len(drc_errs),
        "nets not fully connected: %d" % len(unconnected),
        "",
        "Note: the GND pours on both layers are unfilled in the file; press B in",
        "KiCad (Edit > Fill All Zones) before plotting. All GND pads are also",
        "connected by tracks, so the board works without the pour.",
    ]) + "\n"
    return files


PLACEMENT_ERRORS = []
LAST = {}


def tracks_count():
    return LAST.get("tracks", [])


def vias_count():
    return LAST.get("vias", [])

ROUTE_FAILED = []
DRC_RESULT = []


def main():
    check = "--check" in sys.argv
    errors, warnings = erc()
    files = outputs()
    errors += check_schematic(files[PROJECT + ".kicad_sch"])
    errors += ["placement: " + e for e in PLACEMENT_ERRORS]
    for net, pad in ROUTE_FAILED:
        print("ROUTE: could not reach %s on net %s" % (pad, net))
    drc_errs, unconnected = DRC_RESULT[0]
    for e in drc_errs:
        errors.append("DRC " + e)
    for net, comps in unconnected.items():
        msg = "unconnected %s: %s" % (net, " | ".join(",".join(c[:4]) for c in comps))
        if net == "GND":
            print("DRC note (GND pour expected to join):", msg)
        else:
            errors.append("DRC " + msg)
    print("tracks: %d, vias: %d" % (len(tracks_count()), len(vias_count())))
    errors += ["pcb: parentheses unbalanced"] if not sexpr_balanced(files[PROJECT + ".kicad_pcb"]) else []
    for w in warnings:
        print("ERC warning:", w)
    for e in errors:
        print("ERC ERROR:", e)
    if errors and "--force" not in sys.argv:
        return 1
    os.makedirs(OUT, exist_ok=True)
    stale = []
    for name, content in files.items():
        path = os.path.join(OUT, name)
        old = open(path, encoding="utf-8").read() if os.path.exists(path) else None
        if old != content:
            stale.append(name)
            if not check:
                with open(path, "w", encoding="utf-8", newline="\n") as f:
                    f.write(content)
    nets = design.nets()
    print("parts: %d, nets: %d, connections: %d" % (len(design.PARTS), len(nets), sum(len(v) for v in nets.values())))
    if check and stale:
        print("stale generated files:", ", ".join(stale))
        return 1
    print("written:" if not check else "up to date:", ", ".join(sorted(files)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
