import os
import struct
import zlib
import math

def encode_indexed_png(width, height, palette, raw_indices):
    """
    Encode an 8-bit indexed PNG with PLTE chunk.
    raw_indices is a bytearray of length height * (1 + width),
    where each row starts with a filter byte (0).
    """
    plte = bytearray()
    for r, g, b in palette:
        plte.extend([max(0, min(255, int(r))), max(0, min(255, int(g))), max(0, min(255, int(b)))])
    
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0)
    
    def make_chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)
    
    png = bytearray(b'\x89PNG\r\n\x1a\n')
    png += make_chunk(b'IHDR', ihdr_data)
    png += make_chunk(b'PLTE', plte)
    png += make_chunk(b'IDAT', zlib.compress(raw_indices, 9))
    png += make_chunk(b'IEND', b'')
    return bytes(png)

# ==========================================
# 1. LOGO_PVZ.PNG (46 x 50)
# ==========================================
def generate_logo():
    W, H = 46, 50
    TRANSPARENT = (255, 0, 255)
    pixels = [[TRANSPARENT for _ in range(W)] for _ in range(H)]

    def set_p(x, y, c):
        if 0 <= x < W and 0 <= y < H:
            pixels[y][x] = c

    def draw_circle(cx, cy, r, fill, outline=None):
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                d = math.hypot(x - cx, y - cy)
                if d <= r:
                    if outline and d >= r - 1.0:
                        set_p(x, y, outline)
                    else:
                        set_p(x, y, fill)

    def draw_ellipse(cx, cy, rx, ry, fill, outline=None):
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            for x in range(int(cx - rx - 1), int(cx + rx + 2)):
                d = ((x - cx)/rx)**2 + ((y - cy)/ry)**2
                if d <= 1.0:
                    if outline and d >= 0.72:
                        set_p(x, y, outline)
                    else:
                        set_p(x, y, fill)

    # Ground leaves
    draw_ellipse(13, 44.5, 8.5, 3.5, (45, 145, 35), (15, 60, 15))
    draw_ellipse(32, 44.5, 9.0, 3.5, (45, 145, 35), (15, 60, 15))
    draw_ellipse(22, 46.0, 7.5, 3.0, (55, 170, 40), (15, 60, 15))

    # Leaf veins
    for i in range(5):
        set_p(11 + i, 44, (90, 210, 70))
        set_p(29 + i, 44, (90, 210, 70))

    # Stem
    for y in range(24, 45):
        t = (y - 24) / 21.0
        cx = 20.0 + t * 2.5
        for x in range(int(cx - 2), int(cx + 3)):
            if x == int(cx - 2) or x == int(cx + 2):
                set_p(x, y, (15, 60, 15))
            else:
                set_p(x, y, (60, 180, 45))

    # Head back tendril / leaf
    draw_ellipse(9, 15, 6.5, 3.2, (60, 180, 45), (15, 60, 15))
    set_p(3, 14, (15, 60, 15))
    set_p(4, 14, (60, 180, 45))
    set_p(3, 15, (15, 60, 15))

    # Snout tube
    for x in range(26, 39):
        for y in range(14, 25):
            if y == 14 or y == 24:
                set_p(x, y, (15, 55, 15))
            else:
                grad = int(140 + (20 - abs(y - 19.5)) * 8)
                set_p(x, y, (45, grad, 35))

    # Snout rim / lip (3D flared collar)
    draw_ellipse(39, 19.5, 3.2, 5.5, (15, 45, 15), (15, 55, 15))
    draw_ellipse(39.5, 19.5, 2.0, 4.0, (8, 25, 10))
    # Loaded pea inside snout
    draw_circle(38.5, 19.5, 1.8, (120, 240, 70), (20, 80, 25))
    set_p(38, 19, (220, 255, 180)) # pea specular shine

    # Spherical Head
    for y in range(8, 32):
        for x in range(9, 32):
            d = math.hypot(x - 20.5, y - 19.5)
            if d <= 10.5:
                if d >= 9.5:
                    set_p(x, y, (15, 55, 15))
                else:
                    light = 1.0 - (math.hypot(x - 17, y - 15) / 14.0)
                    light = max(0.0, min(1.0, light))
                    r = int(35 + light * 95)
                    g = int(120 + light * 115)
                    b = int(25 + light * 60)
                    set_p(x, y, (r, g, b))

    # Eyeball
    draw_ellipse(24, 18, 3.2, 4.2, (250, 250, 250), (15, 40, 15))
    # Pupil
    draw_ellipse(24.8, 18, 1.8, 2.8, (15, 20, 15))
    # Eye shine
    set_p(24, 16, (255, 255, 255))
    set_p(24, 17, (255, 255, 255))
    set_p(25, 16, (255, 255, 255))
    # Brow crease
    set_p(22, 13, (25, 80, 25))
    set_p(23, 13, (25, 80, 25))
    set_p(24, 14, (25, 80, 25))

    # Mini Golden Sun badge (bottom left)
    draw_circle(8, 36, 4.2, (255, 220, 20), (200, 140, 0))
    for ang in [0, 45, 90, 135, 180, 225, 270, 315]:
        rad = math.radians(ang)
        px = int(8 + math.cos(rad) * 5.8)
        py = int(36 + math.sin(rad) * 5.8)
        set_p(px, py, (255, 150, 0))
    set_p(7, 35, (255, 255, 210))
    set_p(8, 35, (255, 255, 210))

    # Palette reduction
    palette = [TRANSPARENT]
    color_map = {TRANSPARENT: 0}
    raw_indices = bytearray()
    for y in range(H):
        raw_indices.append(0)
        for x in range(W):
            c = pixels[y][x]
            if c not in color_map:
                color_map[c] = len(palette)
                palette.append(c)
            raw_indices.append(color_map[c])
    return encode_indexed_png(W, H, palette, raw_indices)

# ==========================================
# 2. BANNER_PVZ.PNG (272 x 24)
# ==========================================
def generate_banner():
    W, H = 272, 24
    TRANSPARENT = (255, 0, 255)
    pixels = [[TRANSPARENT for _ in range(W)] for _ in range(H)]

    def set_p(x, y, c):
        if 0 <= x < W and 0 <= y < H:
            pixels[y][x] = c

    # Horizontal guide accent line at row 12 (semi-transparent style: thin line across)
    for x in range(2, W - 2):
        set_p(x, 12, (70, 140, 70))

    # High-quality 7x11 bold font definitions for letters
    # 1: outline, 2: fill, 3: highlight
    glyphs = {
        'P': [
            "111110",
            "122221",
            "1221121",
            "1221121",
            "122221",
            "122110",
            "122100",
            "122100",
            "122100",
            "111100",
        ],
        'L': [
            "111000",
            "122100",
            "122100",
            "122100",
            "122100",
            "122100",
            "122100",
            "1222221",
            "1222221",
            "1111111",
        ],
        'A': [
            "0011100",
            "0122210",
            "1221221",
            "1221221",
            "1221221",
            "1222221",
            "1222221",
            "1221221",
            "1221221",
            "1110111",
        ],
        'N': [
            "11100111",
            "122101221",
            "122211221",
            "122221221",
            "122122221",
            "122112221",
            "122101221",
            "122101221",
            "122101221",
            "11100111",
        ],
        'T': [
            "11111111",
            "122222221",
            "111221111",
            "00122100",
            "00122100",
            "00122100",
            "00122100",
            "00122100",
            "00122100",
            "00111100",
        ],
        'S': [
            "0111110",
            "1222221",
            "1221111",
            "1221000",
            "0122210",
            "0011221",
            "0001221",
            "1111221",
            "1222221",
            "0111110",
        ],
        'V': [
            "11100111",
            "122101221",
            "122101221",
            "122101221",
            "012212210",
            "012212210",
            "001222100",
            "001222100",
            "000121000",
            "000111000",
        ],
        'Z': [
            "11111111",
            "122222221",
            "11111221",
            "00012210",
            "00122100",
            "01221000",
            "12211110",
            "122222221",
            "122222221",
            "11111111",
        ],
        'O': [
            "0111110",
            "1222221",
            "12211221",
            "12211221",
            "12211221",
            "12211221",
            "12211221",
            "12211221",
            "1222221",
            "0111110",
        ],
        'M': [
            "111000111",
            "1221001221",
            "1222112221",
            "1222222221",
            "1221221221",
            "1221001221",
            "1221001221",
            "1221001221",
            "1221001221",
            "111000111",
        ],
        'B': [
            "1111110",
            "1222221",
            "12211221",
            "12211221",
            "12222210",
            "12211221",
            "12211221",
            "12211221",
            "1222221",
            "1111110",
        ],
        'I': [
            "11111",
            "12221",
            "01210",
            "01210",
            "01210",
            "01210",
            "01210",
            "01210",
            "12221",
            "11111",
        ],
        'E': [
            "1111111",
            "12222221",
            "12211111",
            "12210000",
            "12222210",
            "12211110",
            "12210000",
            "12211111",
            "12222221",
            "1111111",
        ],
        '.': [
            "00",
            "00",
            "00",
            "00",
            "00",
            "00",
            "00",
            "11",
            "11",
            "11",
        ]
    }

    def draw_word(start_x, start_y, word, c_outline, c_fill, c_hi):
        curr_x = start_x
        for ch in word:
            if ch == ' ':
                curr_x += 5
                continue
            glyph = glyphs.get(ch)
            if not glyph:
                curr_x += 6
                continue
            gw = len(glyph[0])
            for gy, row in enumerate(glyph):
                for gx, val in enumerate(row):
                    if val == '1':
                        set_p(curr_x + gx, start_y + gy, c_outline)
                    elif val == '2':
                        # Top rows get highlight
                        c = c_hi if gy <= 2 else c_fill
                        set_p(curr_x + gx, start_y + gy, c)
            curr_x += gw + 2
        return curr_x

    # Render "PLANTS" (Green comic font with dark outline)
    x = draw_word(10, 7, "PLANTS", (15, 60, 15), (70, 205, 50), (160, 255, 90))

    # Render "VS." (Fiery orange/red)
    x += 3
    x = draw_word(x, 7, "VS.", (70, 15, 10), (255, 80, 30), (255, 200, 60))

    # Render "ZOMBIES" (Zombie stone-moss-gray)
    x += 3
    x = draw_word(x, 7, "ZOMBIES", (25, 45, 30), (130, 175, 140), (200, 235, 210))

    # Leaf sprout accent on first 'P'
    set_p(8, 6, (15, 60, 15))
    set_p(7, 5, (70, 205, 50))
    set_p(8, 5, (160, 255, 90))
    set_p(7, 4, (15, 60, 15))

    # Sunflower petal accent on 'S'
    set_p(x + 2, 11, (255, 180, 0))
    set_p(x + 3, 11, (255, 220, 0))
    set_p(x + 3, 12, (255, 150, 0))

    # Convert to indexed PNG
    palette = [TRANSPARENT]
    color_map = {TRANSPARENT: 0}
    raw_indices = bytearray()
    for y in range(H):
        raw_indices.append(0)
        for x in range(W):
            c = pixels[y][x]
            if c not in color_map:
                color_map[c] = len(palette)
                palette.append(c)
            raw_indices.append(color_map[c])
    return encode_indexed_png(W, H, palette, raw_indices)

# ==========================================
# 3. BACKGROUND_PVZ.PNG (320 x 240)
# ==========================================
def generate_background():
    W, H = 320, 240
    pixels = [[(0, 0, 0) for _ in range(W)] for _ in range(H)]

    # 5 lawn lanes (each 48 px high)
    # Band A: Vibrant grass, Band B: Deep rich grass
    band_colors = [
        ((42, 138, 48), (48, 152, 54)),   # Band 0 (Light)
        ((32, 110, 38), (36, 120, 42)),   # Band 1 (Dark)
        ((42, 138, 48), (48, 152, 54)),   # Band 2 (Light)
        ((32, 110, 38), (36, 120, 42)),   # Band 3 (Dark)
        ((42, 138, 48), (48, 152, 54)),   # Band 4 (Light)
    ]

    for y in range(H):
        lane = y // 48
        c1, c2 = band_colors[min(lane, 4)]
        for x in range(W):
            # 32px checker tile
            tile_col = (x // 32) % 2
            tile_row = (y // 48) % 2
            base = c2 if (tile_col ^ tile_row) else c1
            
            # Subtle grass texture noise
            noise = ((x * 17 + y * 31) % 7) - 3
            r = max(0, min(255, base[0] + noise))
            g = max(0, min(255, base[1] + noise * 2))
            b = max(0, min(255, base[2] + noise))

            # Header darkening gradient (top 50 pixels)
            if y < 50:
                darken = (50 - y) / 50.0 * 0.45
                r = int(r * (1.0 - darken))
                g = int(g * (1.0 - darken))
                b = int(b * (1.0 - darken))

            pixels[y][x] = (r, g, b)

    # Decorative silhouette art on right side:
    # Silhouette color: dark forest silhouette (18, 55, 22)
    SIL_COLOR = (18, 55, 22)

    def draw_sil_circle(cx, cy, r):
        for sy in range(int(cy - r), int(cy + r + 1)):
            for sx in range(int(cx - r), int(cx + r + 1)):
                if 0 <= sx < W and 0 <= sy < H:
                    if math.hypot(sx - cx, sy - cy) <= r:
                        pixels[sy][sx] = SIL_COLOR

    def draw_sil_rect(rx, ry, rw, rh):
        for sy in range(ry, ry + rh):
            for sx in range(rx, rx + rw):
                if 0 <= sx < W and 0 <= sy < H:
                    pixels[sy][sx] = SIL_COLOR

    # 1. Zombie silhouette walking from right (cx=275, cy=130)
    # Head & hair
    draw_sil_circle(275, 100, 14)
    # Tilted hair strands
    draw_sil_rect(273, 82, 3, 7)
    draw_sil_rect(278, 83, 3, 6)
    # Open jaw
    draw_sil_rect(265, 104, 8, 7)
    # Neck & Body
    draw_sil_rect(270, 114, 12, 32)
    # Coat tails
    draw_sil_rect(268, 144, 16, 12)
    # Outstretched arm towards left
    for ax in range(235, 272):
        ay = int(120 + math.sin((ax - 235) * 0.08) * 4)
        draw_sil_rect(ax, ay, 2, 5)
    # Hand/fingers reaching
    draw_sil_rect(230, 118, 6, 2)
    draw_sil_rect(231, 122, 5, 2)
    # Legs
    draw_sil_rect(271, 156, 4, 30)
    draw_sil_rect(278, 156, 4, 28)
    # Feet
    draw_sil_rect(266, 184, 9, 4)
    draw_sil_rect(275, 182, 8, 4)

    # 2. Peashooter silhouette defending at x=185, y=140
    # Base leaves
    draw_sil_rect(176, 178, 18, 4)
    # Stem
    draw_sil_rect(183, 148, 4, 30)
    # Head
    draw_sil_circle(182, 138, 11)
    # Snout pointing right
    draw_sil_rect(190, 132, 14, 11)
    draw_sil_rect(203, 130, 3, 15)
    # Tendril on back of head
    draw_sil_circle(171, 133, 4)

    # 3. Flying pea in air (between Peashooter and Zombie)
    draw_sil_circle(218, 138, 3.5)

    # 4. Sunflower silhouette beside Peashooter at x=145, y=145
    # Base
    draw_sil_rect(138, 178, 16, 4)
    # Stem
    draw_sil_rect(144, 150, 4, 28)
    # Sunflower head center
    draw_sil_circle(146, 140, 9)
    # Petals around head
    for a in range(0, 360, 36):
        rad = math.radians(a)
        draw_sil_circle(146 + math.cos(rad) * 12, 140 + math.sin(rad) * 12, 3.5)

    # Palette reduction (cluster similar colors into 8-bit palette <= 256 colors)
    # Reduce color resolution: 5 bits per channel (32 levels)
    palette = []
    color_map = {}
    raw_indices = bytearray()
    for y in range(H):
        raw_indices.append(0)
        for x in range(W):
            r, g, b = pixels[y][x]
            # Quantize slightly to ensure < 256 colors
            qr = (r >> 3) << 3
            qg = (g >> 3) << 3
            qb = (b >> 3) << 3
            c = (qr, qg, qb)
            if c not in color_map:
                if len(palette) >= 256:
                    # Find closest existing color
                    best_dist = 99999999
                    best_idx = 0
                    for idx, (pr, pg, pb) in enumerate(palette):
                        dist = (qr - pr)**2 + (qg - pg)**2 + (qb - pb)**2
                        if dist < best_dist:
                            best_dist = dist
                            best_idx = idx
                    color_map[c] = best_idx
                else:
                    color_map[c] = len(palette)
                    palette.append(c)
            raw_indices.append(color_map[c])

    return encode_indexed_png(W, H, palette, raw_indices)

# Execute and write files
logo_data = generate_logo()
with open("themes/default/logo_pvz.png", "wb") as f:
    f.write(logo_data)
print(f"Generated themes/default/logo_pvz.png: {len(logo_data)} bytes")

banner_data = generate_banner()
with open("themes/default/banner_pvz.png", "wb") as f:
    f.write(banner_data)
print(f"Generated themes/default/banner_pvz.png: {len(banner_data)} bytes")

bg_data = generate_background()
with open("themes/default/background_pvz.png", "wb") as f:
    f.write(bg_data)
print(f"Generated themes/default/background_pvz.png: {len(bg_data)} bytes")
