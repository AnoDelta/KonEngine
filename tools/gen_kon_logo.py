#!/usr/bin/env python3
"""Generate kon_logo.h — embedded 32x32 RGBA KonEngine logo.

If logo.png exists in the repo root, converts it to a C array.
Otherwise generates a programmatic "K" icon.

Usage:
    python3 gen_kon_logo.py [output_path] [logo_png_path]
    python3 gen_kon_logo.py src/window/kon_logo.h
    python3 gen_kon_logo.py src/window/kon_logo.h logo.png
"""
import sys, os

W, H = 32, 32

def generate_k_icon():
    """Programmatic 32x32 'K' on dark blue background."""
    pixels = []
    for y in range(H):
        for x in range(W):
            r, g, b, a = 20, 30, 60, 255
            nx, ny = x / (W - 1), y / (H - 1)

            # Left vertical bar
            if 0.15 <= nx <= 0.35 and 0.15 <= ny <= 0.85:
                r, g, b = 80, 160, 255

            # Upper diagonal
            upper_y = 0.5 - (nx - 0.35) * (0.35 / 0.45)
            if 0.35 <= nx <= 0.82 and abs(ny - upper_y) < 0.12:
                r, g, b = 80, 160, 255

            # Lower diagonal
            lower_y = 0.5 + (nx - 0.35) * (0.35 / 0.45)
            if 0.35 <= nx <= 0.82 and abs(ny - lower_y) < 0.12:
                r, g, b = 80, 160, 255

            # Border
            if x == 0 or x == W-1 or y == 0 or y == H-1:
                r, g, b = 40, 60, 100

            pixels.extend([r, g, b, a])
    return pixels

def load_png(path):
    """Load a PNG and resize to 32x32 RGBA. Requires PIL/Pillow."""
    try:
        from PIL import Image
        img = Image.open(path).convert("RGBA").resize((W, H), Image.LANCZOS)
        return list(img.tobytes())
    except ImportError:
        print(f"  [warn] Pillow not installed, using generated logo", file=sys.stderr)
        return None
    except Exception as e:
        print(f"  [warn] Failed to load {path}: {e}", file=sys.stderr)
        return None

def write_header(pixels, out_path):
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w") as f:
        f.write("// Auto-generated at build time — do not edit\n")
        f.write("#pragma once\n")
        f.write(f"static const unsigned int KON_LOGO_WIDTH = {W};\n")
        f.write(f"static const unsigned int KON_LOGO_HEIGHT = {H};\n")
        f.write(f"static const unsigned char KON_LOGO_DATA[{W*H*4}] = {{\n")
        for i in range(0, len(pixels), 16):
            chunk = pixels[i:i+16]
            line = ", ".join(f"0x{b:02x}" for b in chunk)
            comma = "," if i + 16 < len(pixels) else ""
            f.write(f"    {line}{comma}\n")
        f.write("};\n")

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "src/window/kon_logo.h"
    logo_png = sys.argv[2] if len(sys.argv) > 2 else "logo.png"

    pixels = None
    if os.path.isfile(logo_png):
        pixels = load_png(logo_png)

    if pixels is None:
        pixels = generate_k_icon()

    write_header(pixels, out)
