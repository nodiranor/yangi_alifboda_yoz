"""Build the YAY icons from Icon.png as compact PNG-layer .ico files.

Usage: python tools/make_icons.py Icon.png res/app.ico res/app_paused.ico

Layers of 48 px and larger use a 256-color palette PNG, which keeps the 256 px layer near
14 KB instead of ~100 KB. The small layers stay full RGBA so the tray icon is crisp.
The paused icon is only ever shown in the tray, so it stops at 32 px.
"""
import io
import struct
import sys

from PIL import Image, ImageOps


def png_bytes(img, palette):
    if palette:
        img = img.quantize(colors=256, method=Image.FASTOCTREE, dither=Image.NONE)
    buf = io.BytesIO()
    img.save(buf, "PNG", optimize=True)
    return buf.getvalue()


def write_ico(path, source, sizes):
    layers = []
    for size in sizes:
        layer = source.resize((size, size), Image.LANCZOS)
        layers.append((size, png_bytes(layer, palette=size >= 48)))

    header = struct.pack("<HHH", 0, 1, len(layers))
    offset = 6 + 16 * len(layers)
    entries = b""
    blobs = b""
    for size, data in layers:
        dim = 0 if size >= 256 else size
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(data), offset)
        blobs += data
        offset += len(data)
    with open(path, "wb") as f:
        f.write(header + entries + blobs)


src = Image.open(sys.argv[1]).convert("RGBA")
write_ico(sys.argv[2], src, [16, 20, 24, 32, 48, 256])

gray = ImageOps.grayscale(src)
alpha = src.getchannel("A").point(lambda a: int(a * 0.7))
paused = Image.merge("RGBA", (gray, gray, gray, alpha))
write_ico(sys.argv[3], paused, [16, 20, 24, 32])
print("icons written")
