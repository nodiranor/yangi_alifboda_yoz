"""Build the YAY icons from Icon.png as compact PNG-layer .ico files.

Usage: python tools/make_icons.py Icon.png res/app.ico res/app_paused.ico

Icon.png has a transparent background. Before resizing, the source is cleaned:
  * faint stray pixels left over from background removal are dropped,
  * the empty transparent margin is trimmed and the result is centred on a square canvas,
    so the artwork fills as much of a 16 px tray icon as possible.

Layers of 48 px and larger use a 256-color palette PNG, which keeps the 256 px layer near
14 KB instead of ~100 KB. The small layers stay full RGBA so the tray icon is crisp.
The paused icon is only ever shown in the tray, so it stops at 32 px.
"""
import io
import struct
import sys

from PIL import Image, ImageFilter, ImageOps

MARGIN = 0.015  # fraction of the side kept as transparent padding around the artwork


def prepare(src):
    """Remove stray specks, trim transparent margins, return a square RGBA image."""
    alpha = src.getchannel("A")
    solid = alpha.point(lambda v: 255 if v > 200 else 0)
    near_solid = solid.filter(ImageFilter.MaxFilter(15))  # about 7 px around the artwork
    cleaned = Image.composite(alpha, Image.new("L", alpha.size, 0), near_solid)
    cleaned = cleaned.point(lambda v: 0 if v < 8 else v)

    art = src.copy()
    art.putalpha(cleaned)
    box = cleaned.point(lambda v: 255 if v > 10 else 0).getbbox()
    if box is None:
        return art
    art = art.crop(box)

    side = max(art.size)
    pad = int(round(side * MARGIN))
    canvas = Image.new("RGBA", (side + 2 * pad, side + 2 * pad), (0, 0, 0, 0))
    canvas.paste(art, ((canvas.width - art.width) // 2, (canvas.height - art.height) // 2), art)
    return canvas


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


src = prepare(Image.open(sys.argv[1]).convert("RGBA"))
write_ico(sys.argv[2], src, [16, 20, 24, 32, 48, 256])

gray = ImageOps.grayscale(src)
alpha = src.getchannel("A").point(lambda a: int(a * 0.7))
paused = Image.merge("RGBA", (gray, gray, gray, alpha))
write_ico(sys.argv[3], paused, [16, 20, 24, 32])
print("icons written")
