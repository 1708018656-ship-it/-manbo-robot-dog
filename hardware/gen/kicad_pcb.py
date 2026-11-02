"""
KiCad 8 PCB writer + geometry helpers + placement checks + SVG preview.
"""

import math
from xml.sax.saxutils import escape

import footprints
from kicad_sch import uid, q, fmt

CLEARANCE = 0.2
VIA_SIZE, VIA_DRILL = 0.8, 0.4


def rot_pt(x, y, deg):
    """Local footprint point -> offset in board coords (KiCad: CCW, Y down)."""
    a = math.radians(deg)
    return (x * math.cos(a) + y * math.sin(a), -x * math.sin(a) + y * math.cos(a))


class Placed:
    def __init__(self, part, x, y, rot):
        self.part, self.x, self.y, self.rot = part, x, y, rot
        self.fp = footprints.get(part.footprint)

    def pad_abs(self, pad):
        dx, dy = rot_pt(pad.x, pad.y, self.rot)
        return self.x + dx, self.y + dy

    def pad_size_abs(self, pad):
        if int(round(self.rot)) % 180 == 90:
            return pad.h, pad.w
        return pad.w, pad.h

    def body_abs(self):
        x1, y1, x2, y2 = self.fp.body
        pts = [self._abs(x, y) for x, y in ((x1, y1), (x2, y1), (x2, y2), (x1, y2))]
        xs, ys = [p[0] for p in pts], [p[1] for p in pts]
        return min(xs), min(ys), max(xs), max(ys)

    def _abs(self, x, y):
        dx, dy = rot_pt(x, y, self.rot)
        return self.x + dx, self.y + dy

    def pads(self):
        """yield (pad, net, cx, cy, w, h)"""
        for pad in self.fp.pads:
            cx, cy = self.pad_abs(pad)
            w, h = self.pad_size_abs(pad)
            yield pad, self.part.net_of(pad.num), cx, cy, w, h


def place_all(parts, placement):
    out = []
    missing = []
    for p in parts:
        if p.ref not in placement:
            missing.append(p.ref)
            continue
        x, y, r = placement[p.ref]
        out.append(Placed(p, x, y, r))
    return out, missing


def check_placement(placed, board_w, board_h, edge=0.5):
    errs = []
    boxes = [(pl.part.ref, pl.body_abs()) for pl in placed]
    for ref, (x1, y1, x2, y2) in boxes:
        if x1 < edge or y1 < edge or x2 > board_w - edge or y2 > board_h - edge:
            errs.append("%s body outside board (%.1f,%.1f)-(%.1f,%.1f)" % (ref, x1, y1, x2, y2))
    for i in range(len(boxes)):
        for j in range(i + 1, len(boxes)):
            a, b = boxes[i][1], boxes[j][1]
            if a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]:
                ov = (min(a[2], b[2]) - max(a[0], b[0])) * (min(a[3], b[3]) - max(a[1], b[1]))
                if ov > 0.05:
                    errs.append("overlap %s / %s (%.1f mm2)" % (boxes[i][0], boxes[j][0], ov))
    # pad-to-pad clearance between different nets
    pads = []
    for pl in placed:
        for pad, net, cx, cy, w, h in pl.pads():
            pads.append((pl.part.ref, pad.num, net, cx, cy, w, h))
    for i in range(len(pads)):
        for j in range(i + 1, len(pads)):
            a, b = pads[i], pads[j]
            if a[0] == b[0] or (a[2] and a[2] == b[2]):
                continue
            gx = abs(a[3] - b[3]) - (a[5] + b[5]) / 2
            gy = abs(a[4] - b[4]) - (a[6] + b[6]) / 2
            gap = max(gx, gy)
            if gap < CLEARANCE:
                errs.append("pad clearance %s.%s / %s.%s = %.2f mm" % (a[0], a[1], b[0], b[1], gap))
    return errs


def net_table(parts):
    names = sorted({n for p in parts for n in p.nets.values()})
    names.remove("GND")
    names = ["GND"] + names
    return {n: i + 1 for i, n in enumerate(names)}


LAYERS = """  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (32 "B.Adhes" user "B.Adhesive")
    (33 "F.Adhes" user "F.Adhesive")
    (34 "B.Paste" user)
    (35 "F.Paste" user)
    (36 "B.SilkS" user "B.Silkscreen")
    (37 "F.SilkS" user "F.Silkscreen")
    (38 "B.Mask" user)
    (39 "F.Mask" user)
    (40 "Dwgs.User" user "User.Drawings")
    (41 "Cmts.User" user "User.Comments")
    (42 "Eco1.User" user "User.Eco1")
    (43 "Eco2.User" user "User.Eco2")
    (44 "Edge.Cuts" user)
    (45 "Margin" user)
    (46 "B.CrtYd" user "B.Courtyard")
    (47 "F.CrtYd" user "F.Courtyard")
    (48 "B.Fab" user)
    (49 "F.Fab" user)
  )"""


def footprint_sexpr(pl, nets, project_root):
    p, fp = pl.part, pl.fp
    o = []
    o.append('  (footprint %s (layer "F.Cu") (uuid %s) (at %s %s %s)'
             % (q(p.footprint), q(uid("fp", p.ref)), fmt(pl.x), fmt(pl.y), fmt(pl.rot)))
    x1, y1, x2, y2 = fp.body
    hide_ref = " (hide yes)" if not p.pins else ""      # mounting holes: no silk outside the board
    o.append('    (property "Reference" %s (at %s %s %s) (layer "F.SilkS")%s (uuid %s) (effects (font (size 1 1) (thickness 0.15))))'
             % (q(p.ref), fmt((x1 + x2) / 2), fmt(y1 - 0.9), fmt(pl.rot), hide_ref, q(uid("fpref", p.ref))))
    o.append('    (property "Value" %s (at %s %s %s) (layer "F.Fab") (uuid %s) (effects (font (size 1 1) (thickness 0.15))))'
             % (q(p.value), fmt((x1 + x2) / 2), fmt(y2 + 0.9), fmt(pl.rot), q(uid("fpval", p.ref))))
    for k, v in (("Footprint", p.footprint), ("Datasheet", ""), ("Description", p.desc)):
        o.append('    (property %s %s (at 0 0 0) (layer "F.Fab") (hide yes) (uuid %s) (effects (font (size 1 1) (thickness 0.15))))'
                 % (q(k), q(v), q(uid("fpprop", p.ref, k))))
    if p.pins:
        o.append('    (path %s)' % q("/" + uid("sym", p.ref)))
    o.append('    (attr %s)' % ("smd" if fp.smd else ("exclude_from_pos_files exclude_from_bom" if not p.pins else "through_hole")))
    o.append('    (fp_rect (start %s %s) (end %s %s) (stroke (width 0.12) (type solid)) (fill none) (layer "F.SilkS") (uuid %s))'
             % (fmt(x1), fmt(y1), fmt(x2), fmt(y2), q(uid("silk", p.ref))))
    o.append('    (fp_rect (start %s %s) (end %s %s) (stroke (width 0.05) (type solid)) (fill none) (layer "F.CrtYd") (uuid %s))'
             % (fmt(x1 - 0.25), fmt(y1 - 0.25), fmt(x2 + 0.25), fmt(y2 + 0.25), q(uid("crt", p.ref))))
    for i, (a, b, c, d) in enumerate(fp.silk_lines):
        o.append('    (fp_line (start %s %s) (end %s %s) (stroke (width 0.3) (type solid)) (layer "F.SilkS") (uuid %s))'
                 % (fmt(a), fmt(b), fmt(c), fmt(d), q(uid("sl", p.ref, i))))
    # pin-name silkscreen for connectors (assembly aid)
    if p.ref.startswith("J") and len(p.pins) <= 6:
        names = dict(p.pins)
        for pad in fp.pads:
            o.append('    (fp_text user %s (at %s %s %s) (layer "F.SilkS") (uuid %s) (effects (font (size 0.7 0.7) (thickness 0.12))))'
                     % (q(names.get(pad.num, "")), fmt(pad.x + 2.3), fmt(pad.y), fmt(pl.rot), q(uid("pn", p.ref, pad.num))))
    for pad in fp.pads:
        net = p.net_of(pad.num)
        netpart = ' (net %d %s)' % (nets[net], q(net)) if net else ""
        if pad.kind == "smd":
            layers = '"F.Cu" "F.Paste" "F.Mask"'
        else:
            layers = '"*.Cu" "*.Mask"'
        extra = ""
        if pad.shape == "roundrect":
            extra = " (roundrect_rratio 0.25)"
        drill = " (drill %s)" % fmt(pad.drill) if pad.kind != "smd" else ""
        o.append('    (pad %s %s %s (at %s %s %s) (size %s %s)%s (layers %s)%s%s (uuid %s))'
                 % (q(pad.num), pad.kind, pad.shape, fmt(pad.x), fmt(pad.y), fmt(pl.rot),
                    fmt(pad.w), fmt(pad.h), drill, layers, extra, netpart, q(uid("pad", p.ref, pad.num))))
    o.append("  )")
    return "\n".join(o)


def write(parts, placed, board_w, board_h, tracks, vias, title, rev, date, project):
    nets = net_table(parts)
    root = uid("root", project)
    o = ['(kicad_pcb (version 20240108) (generator "manbo_gen") (generator_version "8.0")',
         '  (general (thickness 1.6) (legacy_teardrops no))',
         '  (paper "A4")',
         '  (title_block (title %s) (date %s) (rev %s) (company "manbo-robot-dog"))' % (q(title), q(date), q(rev)),
         LAYERS,
         '  (setup (pad_to_mask_clearance 0) (allow_soldermask_bridges_in_footprints no))',
         '  (net 0 "")']
    for n, i in sorted(nets.items(), key=lambda kv: kv[1]):
        o.append('  (net %d %s)' % (i, q(n)))
    for pl in placed:
        o.append(footprint_sexpr(pl, nets, root))
    # board outline with 2 mm corner radius
    r = 2.0
    W, H = board_w, board_h
    segs = [((r, 0), (W - r, 0)), ((W, r), (W, H - r)), ((W - r, H), (r, H)), ((0, H - r), (0, r))]
    for i, (a, b) in enumerate(segs):
        o.append('  (gr_line (start %s %s) (end %s %s) (stroke (width 0.1) (type default)) (layer "Edge.Cuts") (uuid %s))'
                 % (fmt(a[0]), fmt(a[1]), fmt(b[0]), fmt(b[1]), q(uid("edge", i))))
    k = r - r / math.sqrt(2)
    arcs = [((0, r), (k, k), (r, 0)),
            ((W - r, 0), (W - k, k), (W, r)),
            ((W, H - r), (W - k, H - k), (W - r, H)),
            ((r, H), (k, H - k), (0, H - r))]
    for i, (s_, m_, e_) in enumerate(arcs):
        o.append('  (gr_arc (start %s %s) (mid %s %s) (end %s %s) (stroke (width 0.1) (type default)) (layer "Edge.Cuts") (uuid %s))'
                 % (fmt(s_[0]), fmt(s_[1]), fmt(m_[0]), fmt(m_[1]), fmt(e_[0]), fmt(e_[1]), q(uid("arc", i))))
    o.append('  (gr_text %s (at %s %s 0) (layer "F.SilkS") (uuid %s) (effects (font (size 1.5 1.5) (thickness 0.25)) (justify left)))'
             % (q("MANBO CARRIER V%s  github: manbo-robot-dog" % rev), fmt(30), fmt(88.0), q(uid("brand"))))
    for i, (x1, y1, x2, y2, w, layer, net) in enumerate(tracks):
        o.append('  (segment (start %s %s) (end %s %s) (width %s) (layer %s) (net %d) (uuid %s))'
                 % (fmt(x1), fmt(y1), fmt(x2), fmt(y2), fmt(w), q(layer), nets[net], q(uid("seg", i))))
    for i, (x, y, net) in enumerate(vias):
        o.append('  (via (at %s %s) (size %s) (drill %s) (layers "F.Cu" "B.Cu") (net %d) (uuid %s))'
                 % (fmt(x), fmt(y), fmt(VIA_SIZE), fmt(VIA_DRILL), nets[net], q(uid("via", i))))
    # GND pour on both layers (press B in KiCad to fill before plotting)
    for layer in ("F.Cu", "B.Cu"):
        o.append('  (zone (net %d) (net_name "GND") (layer %s) (uuid %s) (hatch edge 0.5)' % (nets["GND"], q(layer), q(uid("zone", layer))))
        o.append('    (connect_pads (clearance 0.3)) (min_thickness 0.25) (filled_areas_thickness no)')
        o.append('    (fill yes (thermal_gap 0.5) (thermal_bridge_width 0.6))')
        o.append('    (polygon (pts (xy 0.5 0.5) (xy %s 0.5) (xy %s %s) (xy 0.5 %s))))'
                 % (fmt(W - 0.5), fmt(W - 0.5), fmt(H - 0.5), fmt(H - 0.5)))
    o.append(")")
    return "\n".join(o) + "\n"


def preview_svg(placed, board_w, board_h, tracks=(), vias=(), ratsnest=(), scale=8.0):
    W, H = board_w * scale, board_h * scale
    o = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="-20 -20 %d %d" font-family="monospace">'
         % (W + 40, H + 40, W + 40, H + 40),
         '<rect x="0" y="0" width="%.1f" height="%.1f" rx="%.1f" fill="#0b5d3b" stroke="#ccc" stroke-width="2"/>' % (W, H, 2 * scale)]
    for (x1, y1, x2, y2, w, layer, net) in tracks:
        col = "#e8a33d" if layer == "F.Cu" else "#4a90d9"
        o.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="%.1f" stroke-linecap="round" opacity="0.85"/>'
                 % (x1 * scale, y1 * scale, x2 * scale, y2 * scale, col, w * scale))
    for pl in placed:
        x1, y1, x2, y2 = pl.body_abs()
        o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="none" stroke="#f0f0f0" stroke-width="1"/>'
                 % (x1 * scale, y1 * scale, (x2 - x1) * scale, (y2 - y1) * scale))
        for pad, net, cx, cy, w, h in pl.pads():
            if pad.kind == "np_thru_hole":
                o.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="#222"/>' % (cx * scale, cy * scale, w / 2 * scale))
                continue
            col = "#d4af37" if pad.kind != "smd" else "#c0c0c0"
            if pad.shape == "circle":
                o.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="%s"/>' % (cx * scale, cy * scale, w / 2 * scale, col))
            else:
                rr = min(w, h) / 2 * scale if pad.shape == "oval" else (0.25 * min(w, h) * scale if pad.shape == "roundrect" else 0)
                o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="%.1f" fill="%s"/>'
                         % ((cx - w / 2) * scale, (cy - h / 2) * scale, w * scale, h * scale, rr, col))
            if pad.drill:
                o.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="#0b5d3b"/>' % (cx * scale, cy * scale, pad.drill / 2 * scale))
        o.append('<text x="%.1f" y="%.1f" font-size="%.1f" fill="#fff">%s</text>'
                 % (x1 * scale, (y1 - 0.4) * scale, 1.4 * scale, escape(pl.part.ref)))
    for (x, y, net) in vias:
        o.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="#bbb"/><circle cx="%.1f" cy="%.1f" r="%.1f" fill="#333"/>'
                 % (x * scale, y * scale, VIA_SIZE / 2 * scale, x * scale, y * scale, VIA_DRILL / 2 * scale))
    for (x1, y1, x2, y2) in ratsnest:
        o.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#ffff66" stroke-width="1" opacity="0.6"/>'
                 % (x1 * scale, y1 * scale, x2 * scale, y2 * scale))
    o.append("</svg>")
    return "\n".join(o) + "\n"


def ratsnest(placed):
    """Minimum spanning tree per net (for preview and as router order)."""
    byNet = {}
    for pl in placed:
        for pad, net, cx, cy, w, h in pl.pads():
            if net:
                byNet.setdefault(net, []).append((cx, cy))
    lines = []
    for net, pts in byNet.items():
        if len(pts) < 2:
            continue
        inside = [pts[0]]
        rest = pts[1:]
        while rest:
            best = None
            for a in inside:
                for b in rest:
                    d = (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2
                    if best is None or d < best[0]:
                        best = (d, a, b)
            _, a, b = best
            lines.append((a[0], a[1], b[0], b[1], net))
            inside.append(b)
            rest.remove(b)
    return lines
