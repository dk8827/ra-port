# Generates ios/Assets.xcassets/AppIcon.appiconset/AppIcon-1024.png, the original app icon
# (a gold star over radar rings; no game art). Needs Pillow:
#   python3 ios/make_app_icon.py ios/Assets.xcassets/AppIcon.appiconset/AppIcon-1024.png
import math, sys
from PIL import Image, ImageDraw, ImageFilter

S = 1024
SS = 4  # supersample for smooth edges
N = S * SS

img = Image.new("RGB", (N, N))
px = img.load()
# Radial gradient: warm red centre fading to near-black crimson at the corners.
cx, cy = N / 2, N * 0.46
maxd = math.hypot(N / 2, N / 2) * 1.05
inner, outer = (196, 28, 32), (52, 6, 10)
for y in range(0, N, SS):
    for x in range(0, N, SS):
        t = min(1.0, math.hypot(x - cx, y - cy) / maxd) ** 1.2
        c = tuple(int(inner[i] + (outer[i] - inner[i]) * t) for i in range(3))
        for dy in range(SS):
            for dx in range(SS):
                px[x + dx, y + dy] = c

d = ImageDraw.Draw(img, "RGBA")
# Radar rings and cross-hair.
rc = (N / 2, N / 2)
for r, a in ((0.40, 70), (0.30, 55), (0.20, 40)):
    rr = N * r
    d.ellipse([rc[0] - rr, rc[1] - rr, rc[0] + rr, rc[1] + rr], outline=(255, 210, 120, a), width=int(N * 0.006))
d.line([rc[0], N * 0.08, rc[0], N * 0.92], fill=(255, 210, 120, 35), width=int(N * 0.004))
d.line([N * 0.08, rc[1], N * 0.92, rc[1]], fill=(255, 210, 120, 35), width=int(N * 0.004))

# Five-pointed star.
def star(cx, cy, r_outer, r_inner, rot=-math.pi / 2):
    pts = []
    for k in range(10):
        r = r_outer if k % 2 == 0 else r_inner
        a = rot + k * math.pi / 5
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return pts

ro = N * 0.30
shadow = Image.new("RGBA", (N, N), (0, 0, 0, 0))
ImageDraw.Draw(shadow).polygon(star(rc[0], rc[1] + N * 0.02, ro, ro * 0.40), fill=(0, 0, 0, 150))
shadow = shadow.filter(ImageFilter.GaussianBlur(N * 0.02))
img.paste(shadow, (0, 0), shadow)
d = ImageDraw.Draw(img, "RGBA")
d.polygon(star(rc[0], rc[1], ro, ro * 0.40), fill=(240, 186, 64), outline=(120, 70, 10))
# Bevel: lighter upper facets.
outer_pts = star(rc[0], rc[1], ro, ro * 0.40)
# Bevel: one side of every point is lit, the other shaded, the same way round each arm.
for k in range(0, 10, 2):
    tip = outer_pts[k]
    d.polygon([tip, outer_pts[(k + 1) % 10], rc], fill=(255, 230, 150, 120))
    d.polygon([tip, outer_pts[(k - 1) % 10], rc], fill=(150, 90, 10, 60))

img = img.resize((S, S), Image.LANCZOS)
img.save(sys.argv[1], "PNG")
