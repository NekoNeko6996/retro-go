// Target definition
#define RG_TARGET_NAME             "ESP32-S3-ST7735"

// Storage
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDSPI_HOST       SPI3_HOST
#define RG_STORAGE_SDSPI_SPEED      SPI_MASTER_FREQ_20M
// #define RG_STORAGE_SDMMC_HOST       SDMMC_HOST_SLOT_1
// #define RG_STORAGE_SDMMC_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_STORAGE_FLASH_PARTITION  "vfs"

// Audio
#define RG_AUDIO_USE_INT_DAC        0   // 0 = Disable, 1 = GPIO25, 2 = GPIO26, 3 = Both
#define RG_AUDIO_USE_EXT_DAC        1   // 0 = Disable, 1 = Enable

// Video
#define RG_SCREEN_DRIVER            0   // SPI Driver
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             SPI_MASTER_FREQ_20M // ĐÃ HẠ XUỐNG 20MHz
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_WIDTH             160
#define RG_SCREEN_HEIGHT            128
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}
#define RG_SCREEN_INIT()                                                      \
    ILI9341_CMD(0x01); /* SWRESET */                                          \
    ILI9341_CMD(0x11); /* SLPOUT */                                           \
    ILI9341_CMD(0x36, 0xA0); /* MADCTL: Xoay ngang màn hình (Landscape) */    \
    ILI9341_CMD(0x3A, 0x05); /* COLMOD: 16-bit color */                       \
    ILI9341_CMD(0xB1, 0x01, 0x2C, 0x2D);                                      \
    ILI9341_CMD(0xB2, 0x01, 0x2C, 0x2D);                                      \
    ILI9341_CMD(0xB3, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D);                    \
    ILI9341_CMD(0xB4, 0x07);                                                  \
    ILI9341_CMD(0xC0, 0xA2, 0x02, 0x84);                                      \
    ILI9341_CMD(0xC1, 0xC5);                                                  \
    ILI9341_CMD(0xC2, 0x0A, 0x00);                                            \
    ILI9341_CMD(0xC3, 0x8A, 0x2A);                                            \
    ILI9341_CMD(0xC4, 0x8A, 0xEE);                                            \
    ILI9341_CMD(0xC5, 0x0E);                                                  \
    ILI9341_CMD(0x20); /* INVOFF: Thay thành 0x21 (INVON) nếu bị ngược màu */ \
    ILI9341_CMD(0x29); /* DISPON */


// ==========================================
// BÊN TRÁI: ĐIỀU KHIỂN (JOYSTICK & BUTTONS)
// ==========================================
#define RG_GAMEPAD_ADC_MAP {\
    /* Trục Y (Lên/Xuống) - Tạm lấy ngưỡng < 1000 và > 2600 để né vùng tâm 1755 */ \
    {RG_KEY_UP,    ADC_UNIT_1, ADC_CHANNEL_6, ADC_ATTEN_DB_11, 2600, 4096},\
    {RG_KEY_DOWN,  ADC_UNIT_1, ADC_CHANNEL_6, ADC_ATTEN_DB_11, 0,    1000},\
    /* Trục X (Trái/Phải) - Tạm lấy ngưỡng < 1000 và > 2600 để né vùng tâm 1805 */ \
    {RG_KEY_LEFT,  ADC_UNIT_1, ADC_CHANNEL_5, ADC_ATTEN_DB_11, 0,    1000},\
    {RG_KEY_RIGHT, ADC_UNIT_1, ADC_CHANNEL_5, ADC_ATTEN_DB_11, 2600, 4096},\
}

#define RG_GAMEPAD_GPIO_MAP {\
    /* Logic chuẩn: Kéo lên 3.3V (pullup=1), Nhấn xuống GND (level=0) */ \
    {RG_KEY_SELECT, .num = GPIO_NUM_16, .pullup = 1, .level = 0}, /* Nút K */\
    {RG_KEY_START,  .num = GPIO_NUM_17, .pullup = 1, .level = 0}, /* Nút C */\
    {RG_KEY_MENU,   .num = GPIO_NUM_18, .pullup = 1, .level = 0}, /* Nút D */\
    {RG_KEY_OPTION, .num = GPIO_NUM_8,  .pullup = 1, .level = 0}, /* Nút E */\
    {RG_KEY_A,      .num = GPIO_NUM_15, .pullup = 1, .level = 0}, /* Nút A */\
    {RG_KEY_B,      .num = GPIO_NUM_5,  .pullup = 1, .level = 0}, /* Nút B */\
    {RG_KEY_L,      .num = GPIO_NUM_10, .pullup = 1, .level = 0}, /* Nút F */\
}

// Battery
#define RG_BATTERY_DRIVER           0
#define RG_BATTERY_ADC_UNIT         ADC_UNIT_1
#define RG_BATTERY_ADC_CHANNEL      ADC_CHANNEL_3
#define RG_BATTERY_CALC_PERCENT(raw) (((raw) * 2.f - 3500.f) / (4200.f - 3500.f) * 100.f)
#define RG_BATTERY_CALC_VOLTAGE(raw) ((raw) * 2.f * 0.001f)

// ==========================================
// BÊN PHẢI: HIỂN THỊ, LƯU TRỮ & ÂM THANH
// ==========================================
// Status LED
#define RG_GPIO_LED                 GPIO_NUM_NC

// SPI Display (Dải chân bên Phải)
#define RG_GPIO_LCD_MISO            GPIO_NUM_NC
#define RG_GPIO_LCD_MOSI            GPIO_NUM_21 
#define RG_GPIO_LCD_CLK             GPIO_NUM_20
#define RG_GPIO_LCD_CS              GPIO_NUM_45
#define RG_GPIO_LCD_DC              GPIO_NUM_19
#define RG_GPIO_LCD_BCKL            GPIO_NUM_0
#define RG_GPIO_LCD_RST             GPIO_NUM_47

// SPI SD Card (Dải chân bên Phải)
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_42 
#define RG_GPIO_SDSPI_MISO          GPIO_NUM_1
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_2
#define RG_GPIO_SDSPI_CS            GPIO_NUM_41

// External I2S DAC (Dải chân bên Phải)
#define RG_GPIO_SND_I2S_DATA        -1
#define RG_GPIO_SND_I2S_BCK         -1
#define RG_GPIO_SND_I2S_WS          -1