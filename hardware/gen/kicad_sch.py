"""
Emit a KiCad 8 schematic (.kicad_sch) and symbol library (.kicad_sym) from
design.PARTS. Connections are made with a net label on every pin end (no
wires), which keeps the generated sheet readable and unambiguous.
"""

import uuid

GRID = 1.27
PIN_LEN = 2.54
PITCH = 2.54
NS = uuid.UUID("6b1d1f2e-3c4d-4e5f-8a9b-0c1d2e3f4a5b")


def uid(*parts):
    return str(uuid.uuid5(NS, "/".join(str(p) for p in parts)))


def q(s):
    return '"' + str(s).replace("\\", "\\\\").replace('"', '\\"') + '"'


def snap(v):
    return round(round(v / GRID) * GRID, 4)


def fmt(v):
    s = ("%.4f" % v).rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


FONT = "(effects (font (size 1.27 1.27)))"
FONT_HIDE = "(effects (font (size 1.27 1.27)) (hide yes))"


class SymbolShape:
    """Pin geometry for one part (symbol coordinates, Y up)."""

    def __init__(self, part):
        self.part = part
        self.pins = []        # (num, name, x, y, angle)
        names = part.pins
        if len(part.pins) == 40:                # Blue Pill: two rows
            half = len(names) // 2
            self.half_w = 10.16
            top = (half - 1) / 2 * PITCH
            for i, (num, name) in enumerate(names[:half]):
                self.pins.append((num, name, -self.half_w - PIN_LEN, top - i * PITCH, 0))
            for i, (num, name) in enumerate(names[half:]):
                self.pins.append((num, name, self.half_w + PIN_LEN, top - i * PITCH, 180))
            self.half_h = top + PITCH
        else:
            n = len(names)
            self.half_w = 5.08
            top = (n - 1) / 2 * PITCH
            for i, (num, name) in enumerate(names):
                self.pins.append((num, name, -self.half_w - PIN_LEN, snap(top - i * PITCH), 0))
            self.half_h = max(top + PITCH, 2 * PITCH)

    def key(self):
        return (self.part.ref[0], tuple((p[0], p[1]) for p in self.part.pins), len(self.part.pins) == 40)

    def width(self):
        return 2 * (self.half_w + PIN_LEN)

    def height(self):
        return 2 * self.half_h


def lib_symbol(name, shape, prefix):
    out = []
    out.append("    (symbol %s (pin_names (offset 1.016)) (exclude_from_sim no) (in_bom yes) (on_board yes)" % q(name))
    out.append("      (property \"Reference\" %s (at 0 %s 0) %s)" % (q(prefix), fmt(shape.half_h + 1.27), FONT))
    out.append("      (property \"Value\" %s (at 0 %s 0) %s)" % (q(name.split(":")[-1]), fmt(-shape.half_h - 1.27), FONT))
    out.append("      (property \"Footprint\" \"\" (at 0 0 0) %s)" % FONT_HIDE)
    out.append("      (property \"Datasheet\" \"~\" (at 0 0 0) %s)" % FONT_HIDE)
    out.append("      (property \"Description\" \"\" (at 0 0 0) %s)" % FONT_HIDE)
    base = name.split(":")[-1]
    out.append("      (symbol %s" % q(base + "_0_1"))
    out.append("        (rectangle (start %s %s) (end %s %s) (stroke (width 0.254) (type default)) (fill (type background)))"
               % (fmt(-shape.half_w), fmt(shape.half_h), fmt(shape.half_w), fmt(-shape.half_h)))
    out.append("      )")
    out.append("      (symbol %s" % q(base + "_1_1"))
    for num, pname, x, y, ang in shape.pins:
        out.append("        (pin passive line (at %s %s %d) (length %s) (name %s %s) (number %s %s))"
                   % (fmt(x), fmt(y), ang, fmt(PIN_LEN), q(pname), FONT, q(num), FONT))
    out.append("      )")
    out.append("    )")
    return "\n".join(out)


def build(parts, title, project, rev, date, sheet_titles=None, mcu_ref="U3"):
    root = uid("root", project)
    shapes = [SymbolShape(p) for p in parts if p.pins]
    libnames = {}
    lib_defs = []
    for s in shapes:
        k = s.key()
        if k not in libnames:
            base = "".join(ch if ch.isalnum() else "_" for ch in s.part.value)[:24]
            name = "manbo:%s_%d" % (base, len(libnames) + 1)
            libnames[k] = name
            lib_defs.append(lib_symbol(name, s, "".join(c for c in s.part.ref if c.isalpha())))

    # ---- placement: one column per group, top to bottom -------------------
    order = ["power", "mcu", "servo", "modules", "sense"]
    col_x = {"power": 50.8, "mcu": 165.1, "servo": 254.0, "modules": 330.2, "sense": 165.1}
    col_y = {"power": 45.72, "mcu": 45.72, "servo": 45.72, "modules": 45.72, "sense": 165.1}
    titles = {"power": "POWER (2S -> Q2 reverse prot. -> Q1 switch -> 2x buck)",
              "mcu": "MCU: Blue Pill (MANBO_BOARD_V2)",
              "servo": "SERVOS: TIM3 CH1-4",
              "modules": "MODULES",
              "sense": "BATTERY SENSE: VBAT/4.03 -> PA4"}
    if sheet_titles:
        titles.update(sheet_titles)
    present = {p.group if p.group in col_x else "modules" for p in parts if p.pins}
    order = [g for g in order if g in present]

    body = []
    placed = []
    for g in order:
        body.append("  (text %s (exclude_from_sim no) (at %s %s 0) (effects (font (size 2 2) (bold yes)) (justify left bottom)) (uuid %s))"
                    % (q(titles[g]), fmt(col_x[g] - 15.24), fmt(col_y[g] - 10.16), q(uid("title", g))))

    for s in shapes:
        p = s.part
        g = p.group if p.group in col_x else "modules"
        x = snap(col_x[g])
        y = snap(col_y[g] + s.half_h)
        col_y[g] = y + s.half_h + 10.16
        lib = libnames[s.key()]
        placed.append((p, s, x, y))
        body.append("  (symbol (lib_id %s) (at %s %s 0) (unit 1) (exclude_from_sim no) (in_bom yes) (on_board yes) (dnp %s) (uuid %s)"
                    % (q(lib), fmt(x), fmt(y), "yes" if p.dnp else "no", q(uid("sym", p.ref))))
        body.append("    (property \"Reference\" %s (at %s %s 0) %s)" % (q(p.ref), fmt(x), fmt(y - s.half_h - 1.27), FONT))
        body.append("    (property \"Value\" %s (at %s %s 0) %s)" % (q(p.value), fmt(x), fmt(y + s.half_h + 1.27), FONT))
        body.append("    (property \"Footprint\" %s (at %s %s 0) %s)" % (q(p.footprint), fmt(x), fmt(y), FONT_HIDE))
        body.append("    (property \"Datasheet\" \"~\" (at %s %s 0) %s)" % (fmt(x), fmt(y), FONT_HIDE))
        body.append("    (property \"Description\" %s (at %s %s 0) %s)" % (q(p.desc), fmt(x), fmt(y), FONT_HIDE))
        body.append("    (property \"LCSC\" %s (at %s %s 0) %s)" % (q(p.lcsc), fmt(x), fmt(y), FONT_HIDE))
        for num, _n in p.pins:
            body.append("    (pin %s (uuid %s))" % (q(num), q(uid("pin", p.ref, num))))
        body.append("    (instances (project %s (path %s (reference %s) (unit 1))))" % (q(project), q("/" + root), q(p.ref)))
        body.append("  )")
        # labels / no-connects at pin ends (schematic Y = y - pin_y)
        for num, pname, px, py, ang in s.pins:
            ex, ey = snap(x + px), snap(y - py)
            net = p.net_of(num)
            if net:
                if ang == 0:     # pin on the left, label text grows to the left
                    body.append("  (label %s (at %s %s 180) (effects (font (size 1.27 1.27)) (justify right bottom)) (uuid %s))"
                                % (q(net), fmt(ex), fmt(ey), q(uid("lbl", p.ref, num))))
                else:
                    body.append("  (label %s (at %s %s 0) (effects (font (size 1.27 1.27)) (justify left bottom)) (uuid %s))"
                                % (q(net), fmt(ex), fmt(ey), q(uid("lbl", p.ref, num))))
            else:
                body.append("  (no_connect (at %s %s) (uuid %s))" % (fmt(ex), fmt(ey), q(uid("nc", p.ref, num))))

    # mounting holes listed as text (no pins, PCB-only)
    holes = [p.ref for p in parts if not p.pins]
    if holes:
        body.append("  (text %s (exclude_from_sim no) (at 25.4 %s 0) (effects (font (size 1.5 1.5)) (justify left bottom)) (uuid %s))"
                    % (q("Mounting holes (PCB only): " + ", ".join(holes) + " - M3"), fmt(max(col_y.values()) + 5.08), q(uid("holes"))))

    head = ["(kicad_sch (version 20231120) (generator \"manbo_gen\") (generator_version \"8.0\")",
            "  (uuid %s)" % q(root),
            "  (paper \"A2\")",
            "  (title_block (title %s) (date %s) (rev %s) (company \"manbo-robot-dog\")"
            % (q(title), q(date), q(rev)),
            "    (comment 1 \"Generated by hardware/gen/build.py from hardware/gen/design.py - do not edit by hand\"))",
            "  (lib_symbols"]
    tail = ["  (sheet_instances (path \"/\" (page \"1\")))", ")"]
    sch = "\n".join(head + lib_defs + ["  )"] + body + tail) + "\n"

    symlib = "(kicad_symbol_lib (version 20231120) (generator \"manbo_gen\") (generator_version \"8.0\")\n" + \
        "\n".join(d.replace(q(n), q(n.split(":")[1]), 1) for d, n in zip(lib_defs, libnames.values())) + "\n)\n"
    return sch, symlib, placed, {g: (col_x[g], titles[g]) for g in order}
