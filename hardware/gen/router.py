"""
Small 2-layer grid router for the carrier board.

  * grid 0.25 mm, 8-direction moves, vias between F.Cu and B.Cu
  * per-net width + clearance: other nets' copper is dilated by
    (width/2 + clearance) before each net is routed
  * multi-pin nets grow a tree: multi-source A* from all copper of the net
    to the nearest unconnected pad
  * nets routed widest first, then by total ratsnest length

Results are checked independently by drc.py (exact geometry, not grid).
"""

import heapq
import math

import numpy as np
from scipy import ndimage

G = 0.25            # grid pitch, mm
EDGE = 0.6          # copper to board edge
VIA_R = 0.4         # via pad radius (0.8 mm)
VIA_COST = 12.0
LAYER_CHANGE_ONLY_VIA = True
TURN_COST = 0.4


class Router:
    def __init__(self, placed, board_w, board_h, widths, default_width, clearance, power_clearance,
                 power_nets):
        self.W = int(round(board_w / G)) + 1
        self.H = int(round(board_h / G)) + 1
        self.bw, self.bh = board_w, board_h
        self.placed = placed
        self.widths = widths
        self.default_width = default_width
        self.clearance = clearance
        self.power_clearance = power_clearance
        self.power_nets = power_nets
        self.net_ids = {}
        # occupancy: 0 free, >0 net id, -1 blocked for all (holes, edge)
        self.occ = np.zeros((2, self.H, self.W), dtype=np.int32)
        self.pads_by_net = {}
        self.tracks = []    # (x1,y1,x2,y2,w,layer,net)
        self.vias = []      # (x,y,net)
        self.failed = []
        self.merged = {}
        self._paint_edges()
        self._paint_pads()
        self.add_straps()

    # ---------------------------------------------------------- helpers --
    def nid(self, net):
        if net not in self.net_ids:
            self.net_ids[net] = len(self.net_ids) + 1
        return self.net_ids[net]

    def cell(self, x, y):
        return int(round(y / G)), int(round(x / G))

    def width(self, net):
        return self.widths.get(net, self.default_width)

    def clear(self, net):
        return self.power_clearance if net in self.power_nets else self.clearance

    def _paint_edges(self):
        e = int(math.ceil(EDGE / G))
        self.occ[:, :e, :] = -1
        self.occ[:, -e:, :] = -1
        self.occ[:, :, :e] = -1
        self.occ[:, :, -e:] = -1

    def _paint_rect(self, layer, cx, cy, w, h, val, shape="rect"):
        y0, y1 = int(math.floor((cy - h / 2) / G)), int(math.ceil((cy + h / 2) / G))
        x0, x1 = int(math.floor((cx - w / 2) / G)), int(math.ceil((cx + w / 2) / G))
        for yy in range(max(y0, 0), min(y1 + 1, self.H)):
            for xx in range(max(x0, 0), min(x1 + 1, self.W)):
                px, py = xx * G, yy * G
                if abs(px - cx) <= w / 2 + 1e-6 and abs(py - cy) <= h / 2 + 1e-6:
                    if shape == "circle" and (px - cx) ** 2 + (py - cy) ** 2 > (w / 2) ** 2 + 1e-6:
                        continue
                    if self.occ[layer, yy, xx] in (0, val) or val == -1:
                        self.occ[layer, yy, xx] = val

    def add_straps(self):
        """Join adjacent same-net pads of one footprint (e.g. SOIC-8 S/D pins)."""
        for pl in self.placed:
            pads = [(pad, net, cx, cy, w, h) for pad, net, cx, cy, w, h in pl.pads() if net and pad.kind == "smd"]
            for i in range(len(pads)):
                for j in range(i + 1, len(pads)):
                    a, b = pads[i], pads[j]
                    if a[1] != b[1]:
                        continue
                    d = math.hypot(a[2] - b[2], a[3] - b[3])
                    if d <= 1.3:
                        w = min(a[4], a[5], b[4], b[5])
                        self.tracks.append((a[2], a[3], b[2], b[3], w, "F.Cu", a[1]))
                        me = self.nid(a[1])
                        n = int(d / (G / 2)) + 1
                        for k in range(n + 1):
                            x = a[2] + (b[2] - a[2]) * k / n
                            y = a[3] + (b[3] - a[3]) * k / n
                            self._paint_rect(0, x, y, w + G, w + G, me)
                        # pads now form one connected group for routing
                        self.merged.setdefault(a[1], []).append(((pl.part.ref, a[0].num), (pl.part.ref, b[0].num)))

    def _paint_pads(self):
        for pl in self.placed:
            for pad, net, cx, cy, w, h in pl.pads():
                layers = [0] if pad.kind == "smd" else [0, 1]
                if pad.kind == "np_thru_hole":
                    for L in (0, 1):
                        self._paint_rect(L, cx, cy, w + 0.6, h + 0.6, -1, "circle")
                    continue
                val = self.nid(net) if net else -1
                shape = "circle" if pad.shape == "circle" else "rect"
                for L in layers:
                    self._paint_rect(L, cx, cy, w + G, h + G, val, shape)
                if net:
                    self.pads_by_net.setdefault(net, []).append((cx, cy, w, h, tuple(layers), pl.part.ref, pad.num))

    def _blocked(self, net):
        """bool[2,H,W]: True where the centre of a track of this net may not go."""
        me = self.nid(net)
        power_ids = [self.net_ids[n] for n in self.power_nets if n in self.net_ids]

        def disk_for(clear):
            r = self.width(net) / 2 + clear
            rc = int(math.ceil(r / G))
            yy, xx = np.mgrid[-rc:rc + 1, -rc:rc + 1]
            return (xx * G) ** 2 + (yy * G) ** 2 <= (r + G * 0.5) ** 2
        # clearance between two nets = the larger of their classes
        d_sig = disk_for(self.clear(net))
        d_pow = disk_for(max(self.clear(net), self.power_clearance))
        blocked = np.zeros_like(self.occ, dtype=bool)
        for L in (0, 1):
            other = (self.occ[L] != 0) & (self.occ[L] != me)
            is_pow = np.isin(self.occ[L], power_ids) & other
            blocked[L] = ndimage.binary_dilation(other & ~is_pow, structure=d_sig) | \
                ndimage.binary_dilation(is_pow, structure=d_pow)
            # board edge: half track width
            ew = int(math.ceil((EDGE + self.width(net) / 2) / G))
            blocked[L, :ew, :] = True
            blocked[L, -ew:, :] = True
            blocked[L, :, :ew] = True
            blocked[L, :, -ew:] = True
        # via landing needs both layers clear of radius VIA_R + clearance
        rv = VIA_R + max(self.clear(net), self.power_clearance)
        rcv = int(math.ceil(rv / G))
        yy, xx = np.mgrid[-rcv:rcv + 1, -rcv:rcv + 1]
        vdisk = (xx * G) ** 2 + (yy * G) ** 2 <= (rv + G * 0.5) ** 2
        any_other = ((self.occ[0] != 0) & (self.occ[0] != me)) | ((self.occ[1] != 0) & (self.occ[1] != me))
        via_blocked = ndimage.binary_dilation(any_other, structure=vdisk) | blocked[0] | blocked[1]
        return blocked, via_blocked

    def _paint_path(self, cells, net):
        me = self.nid(net)
        r = self.width(net) / 2
        rc = int(math.ceil(r / G))
        offs = [(dy, dx) for dy in range(-rc, rc + 1) for dx in range(-rc, rc + 1)
                if (dx * G) ** 2 + (dy * G) ** 2 <= (r + G * 0.5) ** 2]
        prev = None
        for (L, y, x) in cells:
            if prev is not None and prev[0] != L:
                # via
                rv = int(math.ceil((VIA_R + G * 0.5) / G))
                for LL in (0, 1):
                    for dy in range(-rv, rv + 1):
                        for dx in range(-rv, rv + 1):
                            if (dx * G) ** 2 + (dy * G) ** 2 <= (VIA_R + G * 0.5) ** 2 + 1e-6:
                                if self.occ[LL, y + dy, x + dx] == 0:
                                    self.occ[LL, y + dy, x + dx] = me
            for dy, dx in offs:
                yy, xx = y + dy, x + dx
                if 0 <= yy < self.H and 0 <= xx < self.W and self.occ[L, yy, xx] == 0:
                    self.occ[L, yy, xx] = me
            prev = (L, y, x)

    # ------------------------------------------------------------ A* -----
    def _astar(self, net, sources, targets, blocked, via_blocked):
        """sources/targets: sets of (L,y,x). Returns cell path or None."""
        tgt = targets
        tys = [t[1] for t in tgt]
        txs = [t[2] for t in tgt]
        ty0, ty1, tx0, tx1 = min(tys), max(tys), min(txs), max(txs)

        def h(y, x):
            dy = 0 if ty0 <= y <= ty1 else min(abs(y - ty0), abs(y - ty1))
            dx = 0 if tx0 <= x <= tx1 else min(abs(x - tx0), abs(x - tx1))
            return max(dx, dy) + (math.sqrt(2) - 1) * min(dx, dy)

        dirs = [(0, 1, 1.0), (1, 0, 1.0), (0, -1, 1.0), (-1, 0, 1.0),
                (1, 1, math.sqrt(2)), (1, -1, math.sqrt(2)), (-1, 1, math.sqrt(2)), (-1, -1, math.sqrt(2))]
        best = {}
        came = {}
        heap = []
        for s in sources:
            best[(s, -1)] = 0.0
            heapq.heappush(heap, (h(s[1], s[2]), 0.0, s, -1))
        H, W = self.H, self.W
        expanded = 0
        while heap:
            f, g, cur, d = heapq.heappop(heap)
            if best.get((cur, d), 1e18) < g - 1e-9:
                continue
            if cur in tgt:
                path = [cur]
                key = (cur, d)
                while key in came:
                    key = came[key]
                    path.append(key[0])
                return path[::-1]
            expanded += 1
            if expanded > 400000:
                return None
            L, y, x = cur
            # planar moves
            for di, (dy, dx, c) in enumerate(dirs):
                ny, nx = y + dy, x + dx
                if not (0 <= ny < H and 0 <= nx < W):
                    continue
                nxt = (L, ny, nx)
                if blocked[L, ny, nx] and nxt not in tgt:
                    continue
                if dy and dx:   # no corner cutting
                    if blocked[L, y, nx] or blocked[L, ny, x]:
                        if nxt not in tgt:
                            continue
                ng = g + c + (TURN_COST if d not in (-1, di) else 0.0) + (0.15 if L == 1 else 0.0)
                key = (nxt, di)
                if ng < best.get(key, 1e18) - 1e-9:
                    best[key] = ng
                    came[key] = (cur, d)
                    heapq.heappush(heap, (ng + h(ny, nx), ng, nxt, di))
            # via
            if not via_blocked[y, x]:
                nxt = (1 - L, y, x)
                ng = g + VIA_COST
                key = (nxt, -1)
                if ng < best.get(key, 1e18) - 1e-9:
                    best[key] = ng
                    came[key] = (cur, d)
                    heapq.heappush(heap, (ng + h(y, x), ng, nxt, -1))
        return None

    def _pad_cells(self, pad, blocked=None):
        cx, cy, w, h, layers = pad[:5]
        cells = set()
        y0, y1 = int(math.ceil((cy - h / 2) / G)), int(math.floor((cy + h / 2) / G))
        x0, x1 = int(math.ceil((cx - w / 2) / G)), int(math.floor((cx + w / 2) / G))
        for L in layers:
            for yy in range(y0, y1 + 1):
                for xx in range(x0, x1 + 1):
                    # keep target cells away from the pad rim so the track end is inside
                    if abs(xx * G - cx) <= max(w / 2 - G, 0) + 1e-6 and abs(yy * G - cy) <= max(h / 2 - G, 0) + 1e-6:
                        cells.add((L, yy, xx))
            if not any(c[0] == L for c in cells):
                cy_, cx_ = self.cell(cx, cy)
                cells.add((L, cy_, cx_))
        return cells

    def route_net(self, net):
        pads = self.pads_by_net.get(net, [])
        if len(pads) < 2:
            return True
        blocked, via_blocked = self._blocked(net)
        # group pads joined by straps (union-find on (ref, num))
        parent = {}

        def f(k):
            parent.setdefault(k, k)
            while parent[k] != k:
                parent[k] = parent[parent[k]]
                k = parent[k]
            return k
        for a, b in self.merged.get(net, []):
            parent[f(a)] = f(b)
        groups = {}
        for p in pads:
            groups.setdefault(f((p[5], p[6])), []).append(p)
        glist = list(groups.values())
        tree_cells = set()
        for p in glist[0]:
            tree_cells |= self._pad_cells(p)
        remaining = []
        for g in glist[1:]:
            remaining.append(g)
        ok = True
        while remaining:
            # nearest remaining pad to current tree (by centre distance)
            def dist(g):
                return min((p[0] - c[2] * G) ** 2 + (p[1] - c[1] * G) ** 2 for p in g for c in tree_cells)
            remaining.sort(key=dist)
            tgroup = remaining.pop(0)
            target = tgroup[0]
            tcells = set()
            for p in tgroup:
                tcells |= self._pad_cells(p)
            path = self._astar(net, tree_cells, tcells, blocked, via_blocked)
            if path is None:
                self.failed.append((net, target[5] + "." + target[6]))
                ok = False
                tree_cells |= tcells     # keep going for the rest
                continue
            self._emit(path, net)
            self._paint_path(path, net)
            tree_cells |= set(path) | tcells
            blocked, via_blocked = self._blocked(net)
        return ok

    def _emit(self, path, net):
        w = self.width(net)
        segs = []
        start = path[0]
        prev = path[0]
        prev_dir = None
        for cur in path[1:]:
            if cur[0] != prev[0]:
                if prev != start:
                    segs.append((start, prev))
                self.vias.append((prev[2] * G, prev[1] * G, net))
                start = cur
                prev_dir = None
                prev = cur
                continue
            d = (cur[1] - prev[1], cur[2] - prev[2])
            if prev_dir is not None and d != prev_dir:
                segs.append((start, prev))
                start = prev
            prev_dir = d
            prev = cur
        if prev != start:
            segs.append((start, prev))
        layer_name = {0: "F.Cu", 1: "B.Cu"}
        for a, b in segs:
            self.tracks.append((a[2] * G, a[1] * G, b[2] * G, b[1] * G, w, layer_name[a[0]], net))

    def route_all(self, order):
        for net in order:
            self.route_net(net)
        return self.failed
