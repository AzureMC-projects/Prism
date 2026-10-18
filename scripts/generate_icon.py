#!/usr/bin/env python3
"""Generate a tiny, dependency-free Windows ICO matching assets/prism-icon.svg's geometry."""
import math
import os
import struct

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "prism.ico")
SIZE = 64

def inside_triangle(px, py, a, b, c):
    def sign(p1, p2, p3):
        return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1])
    d1, d2, d3 = sign((px, py), a, b), sign((px, py), b, c), sign((px, py), c, a)
    return not ((d1 < 0 or d2 < 0 or d3 < 0) and (d1 > 0 or d2 > 0 or d3 > 0))

def mix(a, b, t):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))

def pixel(x, y):
    # Dark transparent canvas with a crisp outlined triangular crystal.
    cx, cy = (SIZE - 1) / 2, (SIZE - 1) / 2
    outer = ((cx, 4), (SIZE - 4, SIZE - 7), (4, SIZE - 7))
    inner = ((cx, 9), (SIZE - 9, SIZE - 17), (9, SIZE - 17))
    if not inside_triangle(x, y, *outer):
        return (0, 0, 0, 0)
    if not inside_triangle(x, y, *inner):
        return (14, 16, 34, 255)
    t = min(1.0, max(0.0, y / SIZE))
    cyan, violet, purple = (165, 243, 252), (99, 102, 241), (192, 132, 252)
    if x < cx and y < SIZE - 18:
        rgb = mix(cyan, violet, t)
    elif y > SIZE * .62:
        rgb = mix((99, 102, 241), (79, 70, 229), (x / SIZE))
    else:
        rgb = mix(violet, purple, min(1.0, max(0.0, x / SIZE)))
    # Facet seams.
    if abs(x - cx) < 0.8 and y < SIZE * .78:
        rgb = (248, 250, 252)
    return (*rgb, 255)

def make_ico():
    # ICO DIB rows are bottom-up BGRA. The 1-bit AND mask follows the XOR bitmap.
    rgba = [[pixel(x, y) for x in range(SIZE)] for y in range(SIZE)]
    xor = bytearray()
    for y in range(SIZE - 1, -1, -1):
        for x in range(SIZE):
            r, g, b, a = rgba[y][x]
            xor.extend((b, g, r, a))
    mask_stride = ((SIZE + 31) // 32) * 4
    mask = bytearray(mask_stride * SIZE)
    header = struct.pack("<IIIHHIIIIII", 40, SIZE, SIZE * 2, 1, 32, 0, len(xor), 0, 0, 0, 0)
    dib = header + xor + mask
    directory = struct.pack("<BBBBHHII", SIZE if SIZE < 256 else 0, SIZE if SIZE < 256 else 0, 0, 0, 1, 32, len(dib), 22)
    return struct.pack("<HHH", 0, 1, 1) + directory + dib

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "wb") as f:
    f.write(make_ico())
print(f"Generated {OUT}")
