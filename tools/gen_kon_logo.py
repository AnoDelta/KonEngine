#!/usr/bin/env python3
"""Generate kon_logo.h — embedded KonEngine logo for favicon + splash.

Outputs two images:
  - KON_ICON_*   (32x32)   — window favicon
  - KON_SPLASH_* (256x256) — boot splash screen

If logo.png exists in the repo root, converts it to both sizes.
Otherwise generates a programmatic "K" icon.

Usage:
    python3 gen_kon_logo.py [output_path] [logo_png_path]
    python3 gen_kon_logo.py src/window/kon_logo.h
    python3 gen_kon_logo.py src/window/kon_logo.h logo.png
"""
import sys, os

ICON_SIZE = 32
SPLASH_SIZE = 256

def generate_k_pixels(w, h):
    """Programmatic 'K' on dark blue background at any resolution."""
    pixels = []
    for y in range(h):
        for x in range(w):
            r, g, b, a = 20, 30, 60, 255
            nx, ny = x / (w - 1), y / (h - 1)

            # Left vertical bar
            if 0.15 <= nx <= 0.35 and 0.15 <= ny <= 0.85:
                r, g, b = 80, 160, 255

            # Upper diagonal
            upper_y = 0.5 - (nx - 0.35) * (0.35 / 0.45)
            if 0.35 <= nx <= 0.82 and abs(ny - upper_y) < 0.08:
                r, g, b = 80, 160, 255

            # Lower diagonal
            lower_y = 0.5 + (nx - 0.35) * (0.35 / 0.45)
            if 0.35 <= nx <= 0.82 and abs(ny - lower_y) < 0.08:
                r, g, b = 80, 160, 255

            # Rounded border feel
            if x <= 1 or x >= w-2 or y <= 1 or y >= h-2:
                r, g, b = 40, 60, 100

            pixels.extend([r, g, b, a])
    return pixels

def load_png(path, size):
    """Load a PNG and resize to size x size RGBA. Requires PIL/Pillow."""
    try:
        from PIL import Image
        img = Image.open(path).convert("RGBA").resize((size, size), Image.LANCZOS)
        return list(img.tobytes())
    except ImportError:
        print(f"  [warn] Pillow not installed, using generated logo", file=sys.stderr)
        return None
    except Exception as e:
        print(f"  [warn] Failed to load {path}: {e}", file=sys.stderr)
        return None

def write_array(f, name, w, h, pixels):
    """Write one RGBA pixel array as a C constant."""
    f.write(f"static const unsigned int {name}_WIDTH = {w};\n")
    f.write(f"static const unsigned int {name}_HEIGHT = {h};\n")
    f.write(f"static const unsigned char {name}_DATA[{w*h*4}] = {{\n")
    for i in range(0, len(pixels), 16):
        chunk = pixels[i:i+16]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        comma = "," if i + 16 < len(pixels) else ""
        f.write(f"    {line}{comma}\n")
    f.write("};\n\n")

def write_header(icon_px, splash_px, out_path):
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w") as f:
        f.write("// Auto-generated at build time — do not edit\n")
        f.write("// Remove this entry from .gitignore for 1.0 release\n")
        f.write("#pragma once\n\n")
        write_array(f, "KON_ICON", ICON_SIZE, ICON_SIZE, icon_px)
        write_array(f, "KON_SPLASH", SPLASH_SIZE, SPLASH_SIZE, splash_px)

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "src/window/kon_logo.h"
    logo_png = sys.argv[2] if len(sys.argv) > 2 else "logo.png"

    icon_px = None
    splash_px = None

    if os.path.isfile(logo_png):
        icon_px = load_png(logo_png, ICON_SIZE)
        splash_px = load_png(logo_png, SPLASH_SIZE)

    if icon_px is None:
        icon_px = generate_k_pixels(ICON_SIZE, ICON_SIZE)
    if splash_px is None:
        splash_px = generate_k_pixels(SPLASH_SIZE, SPLASH_SIZE)

    write_header(icon_px, splash_px, out)
