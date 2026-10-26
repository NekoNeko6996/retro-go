#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// RGB565 helper macro
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

// Palette
#define COLOR_BLACK       RGB565(0, 0, 0)
#define COLOR_WHITE       RGB565(255, 255, 255)
#define COLOR_DARK_GRAY   RGB565(45, 45, 45)
#define COLOR_GRAY        RGB565(110, 110, 110)
#define COLOR_LIGHT_GRAY  RGB565(190, 190, 190)
#define COLOR_RED         RGB565(230, 40, 40)
#define COLOR_DARK_RED    RGB565(140, 20, 20)
#define COLOR_GREEN       RGB565(50, 200, 50)
#define COLOR_DARK_GREEN  RGB565(30, 120, 30)
#define COLOR_LIME        RGB565(140, 240, 40)
#define COLOR_BLUE        RGB565(40, 100, 230)
#define COLOR_ICE_BLUE    RGB565(130, 210, 255)
#define COLOR_YELLOW      RGB565(255, 220, 0)
#define COLOR_ORANGE      RGB565(240, 130, 20)
#define COLOR_FIRE_ORANGE RGB565(255, 90, 10)
#define COLOR_BROWN       RGB565(130, 75, 30)
#define COLOR_DARK_BROWN  RGB565(80, 45, 18)
#define COLOR_PURPLE      RGB565(150, 40, 180)
#define COLOR_DARK_PURPLE RGB565(85, 20, 110)
#define COLOR_LAWN_LIGHT  RGB565(75, 175, 55)
#define COLOR_LAWN_DARK   RGB565(60, 150, 45)
#define COLOR_NIGHT_LIGHT RGB565(35, 45, 75)
#define COLOR_NIGHT_DARK  RGB565(25, 32, 55)
#define COLOR_SKY         RGB565(110, 180, 240)
#define COLOR_NIGHT_SKY   RGB565(15, 20, 45)
#define COLOR_ZOMBIE_SKIN RGB565(100, 150, 110)
#define COLOR_ZOMBIE_COAT RGB565(110, 80, 60)
#define COLOR_ZOMBIE_PANT RGB565(50, 70, 110)

#define SCREEN_W 160
#define SCREEN_H 128

// 5x7 Basic ASCII Font (chars ' ' to '~', ASCII 32 to 126)
static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // 42 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}, // 126 ~
};

static inline void pvz_draw_pixel(uint16_t *buf, int x, int y, uint16_t color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        buf[y * SCREEN_W + x] = color;
    }
}

static inline void pvz_fill_rect(uint16_t *buf, int x, int y, int w, int h, uint16_t color) {
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = (x + w) > SCREEN_W ? SCREEN_W : (x + w);
    int y1 = (y + h) > SCREEN_H ? SCREEN_H : (y + h);
    for (int j = y0; j < y1; j++) {
        uint16_t *line = &buf[j * SCREEN_W + x0];
        int count = x1 - x0;
        for (int i = 0; i < count; i++) {
            line[i] = color;
        }
    }
}

static inline void pvz_draw_rect(uint16_t *buf, int x, int y, int w, int h, uint16_t color) {
    pvz_fill_rect(buf, x, y, w, 1, color);
    pvz_fill_rect(buf, x, y + h - 1, w, 1, color);
    pvz_fill_rect(buf, x, y, 1, h, color);
    pvz_fill_rect(buf, x + w - 1, y, 1, h, color);
}

static inline void pvz_draw_char(uint16_t *buf, int x, int y, char c, uint16_t color) {
    if (c < 32 || c > 126) return;
    const uint8_t *glyph = font_5x7[c - 32];
    for (int col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                pvz_draw_pixel(buf, x + col, y + row, color);
            }
        }
    }
}

static inline void pvz_draw_text(uint16_t *buf, int x, int y, const char *text, uint16_t color) {
    int cx = x;
    while (*text) {
        pvz_draw_char(buf, cx, y, *text, color);
        cx += 6;
        text++;
    }
}

// ----------------------------------------------------------------------------
// Custom Retro Sprites
// ----------------------------------------------------------------------------

// Sunflower (12x12)
static inline void pvz_draw_sunflower(uint16_t *buf, int x, int y) {
    pvz_fill_rect(buf, x + 3, y + 1, 6, 10, COLOR_YELLOW);
    pvz_fill_rect(buf, x + 1, y + 3, 10, 6, COLOR_YELLOW);
    pvz_fill_rect(buf, x + 3, y + 3, 6, 6, COLOR_BROWN);
    pvz_draw_pixel(buf, x + 4, y + 5, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 7, y + 5, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 5, y + 7, COLOR_ORANGE);
    pvz_draw_pixel(buf, x + 6, y + 7, COLOR_ORANGE);
    pvz_fill_rect(buf, x + 5, y + 9, 2, 3, COLOR_DARK_GREEN);
    pvz_draw_pixel(buf, x + 3, y + 11, COLOR_DARK_GREEN);
    pvz_draw_pixel(buf, x + 8, y + 11, COLOR_DARK_GREEN);
}

// Peashooter / Snow Pea / Repeater (12x12)
static inline void pvz_draw_peashooter(uint16_t *buf, int x, int y, int type) {
    uint16_t head_col = (type == 1) ? COLOR_ICE_BLUE : COLOR_LIME;
    uint16_t snout_col = (type == 1) ? COLOR_BLUE : COLOR_GREEN;

    pvz_fill_rect(buf, x + 2, y + 2, 7, 6, head_col);
    pvz_draw_pixel(buf, x + 5, y + 3, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 5, y + 4, COLOR_BLACK);
    pvz_fill_rect(buf, x + 8, y + 3, 4, 4, snout_col);
    pvz_draw_pixel(buf, x + 11, y + 4, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 11, y + 5, COLOR_BLACK);
    pvz_fill_rect(buf, x + 4, y + 8, 2, 4, COLOR_DARK_GREEN);
    pvz_fill_rect(buf, x + 2, y + 11, 6, 1, COLOR_DARK_GREEN);

    if (type == 1) { // Snow Pea (Ice spikes)
        pvz_draw_pixel(buf, x + 1, y + 2, COLOR_WHITE);
        pvz_draw_pixel(buf, x + 1, y + 5, COLOR_WHITE);
    } else if (type == 2) { // Repeater (Double leaves on head)
        pvz_fill_rect(buf, x, y + 2, 2, 3, COLOR_DARK_GREEN);
        pvz_fill_rect(buf, x, y + 5, 2, 3, COLOR_DARK_GREEN);
    }
}

// Wall-nut (10x12)
static inline void pvz_draw_wallnut(uint16_t *buf, int x, int y, int hp_pct) {
    pvz_fill_rect(buf, x + 2, y + 1, 6, 10, COLOR_BROWN);
    pvz_fill_rect(buf, x + 1, y + 3, 8, 6, COLOR_BROWN);
    pvz_draw_pixel(buf, x + 3, y + 4, COLOR_WHITE);
    pvz_draw_pixel(buf, x + 4, y + 4, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 6, y + 4, COLOR_WHITE);
    pvz_draw_pixel(buf, x + 7, y + 4, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 4, y + 7, COLOR_DARK_BROWN);
    pvz_draw_pixel(buf, x + 5, y + 7, COLOR_DARK_BROWN);

    if (hp_pct < 66) {
        pvz_draw_pixel(buf, x + 2, y + 5, COLOR_DARK_BROWN);
        pvz_draw_pixel(buf, x + 3, y + 6, COLOR_DARK_BROWN);
    }
    if (hp_pct < 33) {
        pvz_draw_pixel(buf, x + 6, y + 6, COLOR_DARK_BROWN);
        pvz_draw_pixel(buf, x + 7, y + 7, COLOR_DARK_BROWN);
        pvz_draw_pixel(buf, x + 4, y + 8, COLOR_DARK_BROWN);
    }
}

// Cherry Bomb (12x12)
static inline void pvz_draw_cherry(uint16_t *buf, int x, int y, bool flash) {
    uint16_t cherry_col = flash ? COLOR_YELLOW : COLOR_RED;
    pvz_fill_rect(buf, x + 1, y + 5, 5, 5, cherry_col);
    pvz_fill_rect(buf, x + 6, y + 6, 5, 5, cherry_col);
    pvz_draw_pixel(buf, x + 3, y + 6, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 8, y + 7, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 3, y + 4, COLOR_DARK_GREEN);
    pvz_draw_pixel(buf, x + 4, y + 3, COLOR_DARK_GREEN);
    pvz_draw_pixel(buf, x + 5, y + 2, COLOR_DARK_GREEN);
    pvz_draw_pixel(buf, x + 6, y + 2, COLOR_DARK_GREEN);
    pvz_draw_pixel(buf, x + 7, y + 3, COLOR_DARK_GREEN);
    pvz_draw_pixel(buf, x + 8, y + 5, COLOR_DARK_GREEN);
}

// Potato Mine (10x10)
static inline void pvz_draw_potatomine(uint16_t *buf, int x, int y, bool armed) {
    if (!armed) {
        // Underground sprout
        pvz_fill_rect(buf, x + 3, y + 8, 4, 2, COLOR_BROWN);
        pvz_draw_pixel(buf, x + 5, y + 7, COLOR_GREEN);
        pvz_draw_pixel(buf, x + 5, y + 6, COLOR_LIME);
    } else {
        // Armed potato head above ground
        pvz_fill_rect(buf, x + 2, y + 4, 6, 6, COLOR_BROWN);
        pvz_draw_pixel(buf, x + 3, y + 6, COLOR_BLACK);
        pvz_draw_pixel(buf, x + 6, y + 6, COLOR_BLACK);
        // Flashing red antenna bulb
        pvz_draw_pixel(buf, x + 4, y + 2, COLOR_RED);
        pvz_draw_pixel(buf, x + 5, y + 2, COLOR_RED);
        pvz_draw_pixel(buf, x + 4, y + 3, COLOR_GRAY);
        pvz_draw_pixel(buf, x + 5, y + 3, COLOR_GRAY);
    }
}

// Chomper (12x12)
static inline void pvz_draw_chomper(uint16_t *buf, int x, int y, bool chewing) {
    // Body & Stem
    pvz_fill_rect(buf, x + 5, y + 8, 2, 4, COLOR_DARK_GREEN);
    pvz_fill_rect(buf, x + 2, y + 2, 7, 7, COLOR_PURPLE);

    if (chewing) {
        // Closed mouth
        pvz_fill_rect(buf, x + 4, y + 5, 5, 2, COLOR_DARK_PURPLE);
        pvz_draw_pixel(buf, x + 5, y + 3, COLOR_YELLOW);
    } else {
        // Big open jaws with sharp teeth
        pvz_fill_rect(buf, x + 7, y + 3, 4, 5, COLOR_DARK_PURPLE);
        pvz_draw_pixel(buf, x + 7, y + 3, COLOR_WHITE); // top tooth
        pvz_draw_pixel(buf, x + 7, y + 7, COLOR_WHITE); // bottom tooth
        pvz_draw_pixel(buf, x + 9, y + 3, COLOR_WHITE);
        pvz_draw_pixel(buf, x + 9, y + 7, COLOR_WHITE);
        pvz_draw_pixel(buf, x + 4, y + 3, COLOR_YELLOW); // eye
    }
}

// Jalapeno (10x12)
static inline void pvz_draw_jalapeno(uint16_t *buf, int x, int y) {
    // Chili Pepper Body
    pvz_fill_rect(buf, x + 3, y + 3, 4, 8, COLOR_RED);
    pvz_fill_rect(buf, x + 4, y + 10, 2, 2, COLOR_DARK_RED);
    // Fiery Eyes
    pvz_draw_pixel(buf, x + 3, y + 5, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 6, y + 5, COLOR_BLACK);
    // Green stem
    pvz_draw_pixel(buf, x + 4, y + 2, COLOR_GREEN);
    pvz_draw_pixel(buf, x + 5, y + 1, COLOR_DARK_GREEN);
}

// Shovel icon (10x10)
static inline void pvz_draw_shovel(uint16_t *buf, int x, int y) {
    pvz_fill_rect(buf, x + 5, y + 5, 4, 4, COLOR_LIGHT_GRAY);
    pvz_draw_pixel(buf, x + 4, y + 4, COLOR_BROWN);
    pvz_draw_pixel(buf, x + 3, y + 3, COLOR_BROWN);
    pvz_draw_pixel(buf, x + 2, y + 2, COLOR_BROWN);
    pvz_fill_rect(buf, x + 1, y + 1, 2, 2, COLOR_RED);
}

// Sun sprite (10x10)
static inline void pvz_draw_sun(uint16_t *buf, int x, int y) {
    pvz_fill_rect(buf, x + 2, y, 6, 10, COLOR_ORANGE);
    pvz_fill_rect(buf, x, y + 2, 10, 6, COLOR_ORANGE);
    pvz_fill_rect(buf, x + 2, y + 2, 6, 6, COLOR_YELLOW);
    pvz_draw_pixel(buf, x + 4, y + 4, COLOR_WHITE);
}

// Pea projectile (4x4)
static inline void pvz_draw_pea(uint16_t *buf, int x, int y, bool is_ice) {
    uint16_t col = is_ice ? COLOR_ICE_BLUE : COLOR_LIME;
    pvz_fill_rect(buf, x, y, 4, 4, col);
    pvz_draw_pixel(buf, x + 1, y + 1, COLOR_WHITE);
}

// Lawn Mower (12x10)
static inline void pvz_draw_mower(uint16_t *buf, int x, int y) {
    pvz_fill_rect(buf, x + 2, y + 2, 8, 5, COLOR_RED);
    pvz_fill_rect(buf, x + 4, y, 4, 3, COLOR_DARK_GRAY);
    pvz_fill_rect(buf, x + 1, y + 6, 3, 3, COLOR_BLACK);
    pvz_fill_rect(buf, x + 8, y + 6, 3, 3, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 2, y + 7, COLOR_LIGHT_GRAY);
    pvz_draw_pixel(buf, x + 9, y + 7, COLOR_LIGHT_GRAY);
    pvz_draw_pixel(buf, x + 1, y + 3, COLOR_LIGHT_GRAY);
    pvz_draw_pixel(buf, x, y + 2, COLOR_LIGHT_GRAY);
}

// Gravestone (10x12)
static inline void pvz_draw_gravestone(uint16_t *buf, int x, int y) {
    pvz_fill_rect(buf, x + 2, y + 1, 6, 10, COLOR_GRAY);
    pvz_fill_rect(buf, x + 1, y + 3, 8, 8, COLOR_GRAY);
    pvz_fill_rect(buf, x + 4, y + 3, 2, 6, COLOR_DARK_GRAY);
    pvz_fill_rect(buf, x + 2, y + 5, 6, 2, COLOR_DARK_GRAY);
}

// Zombie sprite (Types: 0:Normal, 1:Cone, 2:Bucket, 3:Flag, 4:Football, 5:Newspaper, 6:Vaulting)
static inline void pvz_draw_zombie(uint16_t *buf, int x, int y, int type, bool is_slowed, int anim_step, bool special_flag) {
    uint16_t skin = is_slowed ? COLOR_ICE_BLUE : COLOR_ZOMBIE_SKIN;

    // Head
    pvz_fill_rect(buf, x + 2, y + 2, 6, 5, skin);
    pvz_draw_pixel(buf, x + 3, y + 3, COLOR_WHITE);
    pvz_draw_pixel(buf, x + 3, y + 4, (type == 5 && special_flag) ? COLOR_RED : COLOR_BLACK);
    pvz_draw_pixel(buf, x + 2, y + 5, COLOR_BLACK);
    pvz_draw_pixel(buf, x + 3, y + 5, COLOR_BLACK);

    // Headwear / Accessories
    if (type == 1) { // Conehead
        pvz_fill_rect(buf, x + 3, y - 4, 4, 6, COLOR_ORANGE);
        pvz_draw_pixel(buf, x + 4, y - 5, COLOR_ORANGE);
        pvz_fill_rect(buf, x + 2, y + 1, 6, 1, COLOR_ORANGE);
    } else if (type == 2) { // Buckethead
        pvz_fill_rect(buf, x + 2, y - 4, 6, 6, COLOR_GRAY);
        pvz_draw_pixel(buf, x + 2, y - 3, COLOR_WHITE);
        pvz_fill_rect(buf, x + 1, y + 1, 8, 1, COLOR_DARK_GRAY);
    } else if (type == 3) { // Flag Zombie
        pvz_fill_rect(buf, x + 8, y - 6, 6, 5, COLOR_RED);
        pvz_draw_pixel(buf, x + 10, y - 4, COLOR_WHITE);
        pvz_fill_rect(buf, x + 7, y - 6, 1, 10, COLOR_DARK_BROWN);
    } else if (type == 4) { // Football Zombie (Red Helmet)
        pvz_fill_rect(buf, x + 1, y - 3, 8, 6, COLOR_DARK_RED);
        pvz_fill_rect(buf, x + 4, y - 3, 2, 6, COLOR_WHITE); // White stripe
        pvz_fill_rect(buf, x + 1, y + 2, 2, 3, COLOR_WHITE); // Face cage
    } else if (type == 5) { // Newspaper Zombie
        pvz_fill_rect(buf, x + 2, y + 1, 6, 2, COLOR_WHITE); // White hair
        if (!special_flag) { // Holding newspaper
            pvz_fill_rect(buf, x - 3, y + 6, 6, 6, COLOR_WHITE);
            pvz_draw_rect(buf, x - 3, y + 6, 6, 6, COLOR_BLACK);
            pvz_draw_pixel(buf, x - 1, y + 8, COLOR_BLACK);
        }
    } else if (type == 6) { // Pole Vaulting Zombie
        pvz_fill_rect(buf, x + 3, y - 1, 4, 2, COLOR_ORANGE); // Headband
        if (!special_flag) { // Carrying pole
            pvz_fill_rect(buf, x - 6, y + 4, 16, 2, COLOR_YELLOW);
        }
    }

    // Body Clothing
    uint16_t coat_col = (type == 4) ? COLOR_RED : COLOR_ZOMBIE_COAT;
    pvz_fill_rect(buf, x + 2, y + 7, 6, 5, coat_col);
    if (type != 4) {
        pvz_draw_pixel(buf, x + 4, y + 7, COLOR_RED); // Tie
        pvz_draw_pixel(buf, x + 4, y + 8, COLOR_RED);
    }

    // Hands
    if (type != 5 || special_flag) {
        pvz_fill_rect(buf, x - 1, y + 7, 3, 2, skin);
    }

    // Pants & Legs
    uint16_t pant_col = (type == 4) ? COLOR_WHITE : COLOR_ZOMBIE_PANT;
    pvz_fill_rect(buf, x + 2, y + 12, 6, 3, pant_col);
    if ((anim_step / 8) % 2 == 0) {
        pvz_fill_rect(buf, x + 2, y + 14, 2, 2, COLOR_BLACK);
    } else {
        pvz_fill_rect(buf, x + 5, y + 14, 2, 2, COLOR_BLACK);
    }
}
