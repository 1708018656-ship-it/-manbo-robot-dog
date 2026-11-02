"""SVG previews (viewable on GitHub / phone without KiCad)."""

from xml.sax.saxutils import escape

NET_COLORS = {"GND": "#555", "+5V": "#c0392b", "+5V_SERVO": "#d35400", "+3V3": "#8e44ad",
              "VBAT": "#b7950b", "VBAT_RP": "#b7950b", "VBAT_F": "#b7950b", "VBAT_IN": "#b7950b"}


def schematic_svg(placed, titles, scale=4.0):
    xs, ys = [], []
    for p, s, x, y in placed:
        xs += [x - s.width() / 2 - 30, x + s.width() / 2 + 30]
        ys += [y - s.half_h - 5, y + s.half_h + 5]
    minx, miny = min(xs) - 10, min(ys) - 25
    w, h = (max(xs) - minx + 10) * scale, (max(ys) - miny + 10) * scale

    def X(v):
        return (v - minx) * scale

    def Y(v):
        return (v - miny) * scale
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d" '
           'font-family="monospace">' % (w, h, w, h),
           '<rect width="100%" height="100%" fill="#fffdf6"/>']
    for g, (cx, title) in titles.items():
        gy = min(y - s.half_h for p, s, x, y in placed if (p.group or "modules") == g) - 12
        out.append('<text x="%.1f" y="%.1f" font-size="%.1f" font-weight="bold" fill="#1a5276">%s</text>'
                   % (X(cx - 15), Y(gy), 2.6 * scale, escape(title)))
    for p, s, x, y in placed:
        out.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="#fef9e7" stroke="#7d6608" stroke-width="2"/>'
                   % (X(x - s.half_w), Y(y - s.half_h), 2 * s.half_w * scale, 2 * s.half_h * scale))
        out.append('<text x="%.1f" y="%.1f" font-size="%.1f" text-anchor="middle" font-weight="bold">%s</text>'
                   % (X(x), Y(y - s.half_h - 1), 1.8 * scale, escape(p.ref)))
        out.append('<text x="%.1f" y="%.1f" font-size="%.1f" text-anchor="middle" fill="#555">%s</text>'
                   % (X(x), Y(y + s.half_h + 2.6), 1.5 * scale, escape(p.value)))
        for num, name, px, py, ang in s.pins:
            ex, ey = x + px, y - py
            bx = ex + (2.54 if ang == 0 else -2.54)
            out.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#7d6608" stroke-width="1.5"/>'
                       % (X(ex), Y(ey), X(bx), Y(ey)))
            inner = bx + (0.8 if ang == 0 else -0.8)
            out.append('<text x="%.1f" y="%.1f" font-size="%.1f" text-anchor="%s">%s</text>'
                       % (X(inner), Y(ey + 0.45), 1.2 * scale, "start" if ang == 0 else "end", escape(name)))
            net = p.net_of(num)
            if net:
                col = NET_COLORS.get(net, "#1e8449")
                lx = ex - 0.8 if ang == 0 else ex + 0.8
                out.append('<text x="%.1f" y="%.1f" font-size="%.1f" text-anchor="%s" fill="%s">%s</text>'
                           % (X(lx), Y(ey + 0.45), 1.3 * scale, "end" if ang == 0 else "start", col, escape(net)))
            else:
                out.append('<text x="%.1f" y="%.1f" font-size="%.1f" text-anchor="middle" fill="#999">x</text>'
                           % (X(ex), Y(ey + 0.45), 1.3 * scale))
    out.append("</svg>")
    return "\n".join(out) + "\n"
