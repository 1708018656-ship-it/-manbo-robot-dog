"""
Independent geometric DRC for the generated board (exact geometry, no grid):

  * clearance between copper of different nets (tracks, vias, pads)
  * copper to board edge
  * connectivity: every net's pads must be joined by its own copper
    (pads, tracks, vias), per layer, through vias / plated holes

Pads are modelled as rectangles (oval/roundrect conservatively as rects,
circles as circles); tracks as capsules.
"""

import math


def _seg_pt(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0.0 if L == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / L))
    cx, cy = ax + t * dx, ay + t * dy
    return math.hypot(px - cx, py - cy)


def _seg_seg(a, b):
    (ax, ay, bx, by), (cx, cy, dx, dy) = a, b
    # proper intersection?
    def orient(px, py, qx, qy, rx, ry):
        v = (qx - px) * (ry - py) - (qy - py) * (rx - px)
        return 0 if abs(v) < 1e-12 else (1 if v > 0 else -1)
    o1, o2 = orient(ax, ay, bx, by, cx, cy), orient(ax, ay, bx, by, dx, dy)
    o3, o4 = orient(cx, cy, dx, dy, ax, ay), orient(cx, cy, dx, dy, bx, by)
    if o1 != o2 and o3 != o4 and 0 not in (o1, o2, o3, o4):
        return 0.0
    return min(_seg_pt(ax, ay, cx, cy, dx, dy), _seg_pt(bx, by, cx, cy, dx, dy),
               _seg_pt(cx, cy, ax, ay, bx, by), _seg_pt(dx, dy, ax, ay, bx, by))


def _rect_seg(rx, ry, w, h, seg):
    """distance from axis-aligned rect to segment (0 if touching)."""
    ax, ay, bx, by = seg
    x1, y1, x2, y2 = rx - w / 2, ry - h / 2, rx + w / 2, ry + h / 2
    if (x1 <= ax <= x2 and y1 <= ay <= y2) or (x1 <= bx <= x2 and y1 <= by <= y2):
        return 0.0
    edges = [(x1, y1, x2, y1), (x2, y1, x2, y2), (x2, y2, x1, y2), (x1, y2, x1, y1)]
    return min(_seg_seg(e, seg) for e in edges)


class Obj:
    __slots__ = ("kind", "net", "layers", "geom", "r", "label")

    def __init__(self, kind, net, layers, geom, r, label):
        self.kind, self.net, self.layers, self.geom, self.r, self.label = kind, net, layers, geom, r, label


def _objects(placed, tracks, vias, via_r):
    objs = []
    for pl in placed:
        for pad, net, cx, cy, w, h in pl.pads():
            layers = {0} if pad.kind == "smd" else {0, 1}
            if pad.kind == "np_thru_hole":
                objs.append(Obj("hole", None, {0, 1}, (cx, cy), w / 2, pl.part.ref))
                continue
            if pad.shape == "circle":
                objs.append(Obj("circle", net, layers, (cx, cy), w / 2, "%s.%s" % (pl.part.ref, pad.num)))
            else:
                objs.append(Obj("rect", net, layers, (cx, cy, w, h), 0, "%s.%s" % (pl.part.ref, pad.num)))
    for i, (x1, y1, x2, y2, w, layer, net) in enumerate(tracks):
        objs.append(Obj("seg", net, {0 if layer == "F.Cu" else 1}, (x1, y1, x2, y2), w / 2, "track#%d" % i))
    for i, (x, y, net) in enumerate(vias):
        objs.append(Obj("circle", net, {0, 1}, (x, y), via_r, "via#%d" % i))
    return objs


def _dist(a, b):
    """edge-to-edge distance (negative/zero = overlap)."""
    ka, kb = a.kind, b.kind
    if ka == "hole":
        a = Obj("circle", None, a.layers, a.geom, a.r, a.label); ka = "circle"
    if kb == "hole":
        b = Obj("circle", None, b.layers, b.geom, b.r, b.label); kb = "circle"
    if ka == "seg" and kb == "seg":
        return _seg_seg(a.geom, b.geom) - a.r - b.r
    if ka == "seg":
        a, b, ka, kb = b, a, kb, ka
    if kb == "seg":
        if ka == "circle":
            x, y = a.geom
            return _seg_pt(x, y, *b.geom) - a.r - b.r
        return _rect_seg(*a.geom, b.geom) - b.r
    if ka == "circle" and kb == "circle":
        return math.hypot(a.geom[0] - b.geom[0], a.geom[1] - b.geom[1]) - a.r - b.r
    if ka == "circle":
        a, b, ka, kb = b, a, kb, ka
    if kb == "circle":   # rect vs circle
        rx, ry, w, h = a.geom
        x, y = b.geom
        dx = max(abs(x - rx) - w / 2, 0)
        dy = max(abs(y - ry) - h / 2, 0)
        return math.hypot(dx, dy) - b.r
    ax, ay, aw, ah = a.geom
    bx, by, bw, bh = b.geom
    dx = max(abs(ax - bx) - (aw + bw) / 2, 0)
    dy = max(abs(ay - by) - (ah + bh) / 2, 0)
    if dx == 0 and dy == 0:
        return -min((aw + bw) / 2 - abs(ax - bx), (ah + bh) / 2 - abs(ay - by))
    return math.hypot(dx, dy)


def _bbox(o):
    if o.kind == "seg":
        x1, y1, x2, y2 = o.geom
        return min(x1, x2) - o.r, min(y1, y2) - o.r, max(x1, x2) + o.r, max(y1, y2) + o.r
    if o.kind in ("circle", "hole"):
        x, y = o.geom
        return x - o.r, y - o.r, x + o.r, y + o.r
    x, y, w, h = o.geom
    return x - w / 2, y - h / 2, x + w / 2, y + h / 2


def run(placed, tracks, vias, board_w, board_h, clearance_of, via_r=0.4, edge=0.5):
    objs = _objects(placed, tracks, vias, via_r)
    errors = []
    boxes = [_bbox(o) for o in objs]
    # edge clearance (tracks/vias only; pads are placed by hand)
    for o, bb in zip(objs, boxes):
        if o.kind in ("seg",) or o.label.startswith("via"):
            if bb[0] < edge or bb[1] < edge or bb[2] > board_w - edge or bb[3] > board_h - edge:
                errors.append("edge clearance: %s (%s)" % (o.label, o.net))
    # pairwise clearance with a coarse spatial hash
    cell = 5.0
    buckets = {}
    for i, bb in enumerate(boxes):
        for gx in range(int(bb[0] // cell), int(bb[2] // cell) + 1):
            for gy in range(int(bb[1] // cell), int(bb[3] // cell) + 1):
                buckets.setdefault((gx, gy), []).append(i)
    seen = set()
    worst = {}
    for idxs in buckets.values():
        for ii in range(len(idxs)):
            for jj in range(ii + 1, len(idxs)):
                i, j = idxs[ii], idxs[jj]
                if (i, j) in seen:
                    continue
                seen.add((i, j))
                a, b = objs[i], objs[j]
                if not (a.layers & b.layers):
                    continue
                if a.net is not None and a.net == b.net:
                    continue
                if a.kind == "rect" and b.kind == "rect" and a.label.split(".")[0] == b.label.split(".")[0]:
                    continue    # pads inside one footprint are the footprint's business
                need = max(clearance_of(a.net), clearance_of(b.net))
                d = _dist(a, b)
                if d < need - 1e-3:
                    key = tuple(sorted((a.label, b.label)))
                    worst[key] = min(worst.get(key, 1e9), d)
    for (la, lb), d in sorted(worst.items(), key=lambda kv: kv[1]):
        errors.append("clearance %.3f mm: %s / %s" % (d, la, lb))

    # connectivity per net (union-find over touching same-net objects)
    parent = list(range(len(objs)))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    for idxs in buckets.values():
        for ii in range(len(idxs)):
            for jj in range(ii + 1, len(idxs)):
                i, j = idxs[ii], idxs[jj]
                a, b = objs[i], objs[j]
                if a.net is None or a.net != b.net or not (a.layers & b.layers):
                    continue
                if _dist(a, b) <= 1e-6:
                    parent[find(i)] = find(j)
    groups = {}
    for i, o in enumerate(objs):
        if o.net and o.kind in ("rect", "circle") and not o.label.startswith("via"):
            groups.setdefault(o.net, {}).setdefault(find(i), []).append(o.label)
    unconnected = {}
    for net, comps in groups.items():
        if len(comps) > 1:
            unconnected[net] = sorted(comps.values(), key=len, reverse=True)
    return errors, unconnected
