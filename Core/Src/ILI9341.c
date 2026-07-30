#include "ILI9341.h"
#include "font5x7.h"
#include <string.h>

extern SPI_HandleTypeDef hspi1;

/* Two different DC/RESET pin labels for the TFT are defined in main.h
 * (must have been assigned twice at some point in CubeMX): TFT_A0/TFT_RESET
 * (PA2/PA3) and TFT_DC/TFT_RST (PB6/PC7). Leave whichever one is physically
 * wired to the module active, and comment out the other. */
#define ILI9341_CS_PORT   TFT_CS_GPIO_Port
#define ILI9341_CS_PIN    TFT_CS_Pin

#define ILI9341_DC_PORT   TFT_DC_GPIO_Port
#define ILI9341_DC_PIN    TFT_DC_Pin
// #define ILI9341_DC_PORT   TFT_A0_GPIO_Port
// #define ILI9341_DC_PIN    TFT_A0_Pin

#define ILI9341_RST_PORT  TFT_RST_GPIO_Port
#define ILI9341_RST_PIN   TFT_RST_Pin
// #define ILI9341_RST_PORT  TFT_RESET_GPIO_Port
// #define ILI9341_RST_PIN   TFT_RESET_Pin

static void ILI9341_Select(void) {
    HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_RESET);
}

static void ILI9341_Deselect(void) {
    HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_SET);
}

static void ILI9341_Reset(void) {
    HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(120);
}

static void ILI9341_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
}

static void ILI9341_WriteData(const uint8_t *data, size_t len) {
    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_SET);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, (uint16_t)len, HAL_MAX_DELAY);
}

void ILI9341_Init(void) {
    ILI9341_Deselect();
    ILI9341_Reset();

    ILI9341_Select();

    ILI9341_WriteCommand(0xEF);
    ILI9341_WriteData((uint8_t[]){0x03, 0x80, 0x02}, 3);

    ILI9341_WriteCommand(0xCF);
    ILI9341_WriteData((uint8_t[]){0x00, 0xC1, 0x30}, 3);

    ILI9341_WriteCommand(0xED);
    ILI9341_WriteData((uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4);

    ILI9341_WriteCommand(0xE8);
    ILI9341_WriteData((uint8_t[]){0x85, 0x00, 0x78}, 3);

    ILI9341_WriteCommand(0xCB);
    ILI9341_WriteData((uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5);

    ILI9341_WriteCommand(0xF7);
    ILI9341_WriteData((uint8_t[]){0x20}, 1);

    ILI9341_WriteCommand(0xEA);
    ILI9341_WriteData((uint8_t[]){0x00, 0x00}, 2);

    ILI9341_WriteCommand(0xC0); /* Power control 1 */
    ILI9341_WriteData((uint8_t[]){0x23}, 1);

    ILI9341_WriteCommand(0xC1); /* Power control 2 */
    ILI9341_WriteData((uint8_t[]){0x10}, 1);

    ILI9341_WriteCommand(0xC5); /* VCOM control 1 */
    ILI9341_WriteData((uint8_t[]){0x3E, 0x28}, 2);

    ILI9341_WriteCommand(0xC7); /* VCOM control 2 */
    ILI9341_WriteData((uint8_t[]){0x86}, 1);

    ILI9341_WriteCommand(0x36); /* Memory Access Control: MX=1, BGR=1 (portrait) */
    ILI9341_WriteData((uint8_t[]){0x48}, 1);

    ILI9341_WriteCommand(0x3A); /* Pixel format: 16 bits/pixel */
    ILI9341_WriteData((uint8_t[]){0x55}, 1);

    ILI9341_WriteCommand(0xB1); /* Frame rate control */
    ILI9341_WriteData((uint8_t[]){0x00, 0x18}, 2);

    ILI9341_WriteCommand(0xB6); /* Display Function Control */
    ILI9341_WriteData((uint8_t[]){0x08, 0x82, 0x27}, 3);

    ILI9341_WriteCommand(0xF2); /* 3Gamma Function Disable */
    ILI9341_WriteData((uint8_t[]){0x00}, 1);

    ILI9341_WriteCommand(0x26); /* Gamma curve select */
    ILI9341_WriteData((uint8_t[]){0x01}, 1);

    ILI9341_WriteCommand(0xE0); /* Positive Gamma Correction */
    ILI9341_WriteData((uint8_t[]){0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E,
                                   0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}, 15);

    ILI9341_WriteCommand(0xE1); /* Negative Gamma Correction */
    ILI9341_WriteData((uint8_t[]){0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31,
                                   0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}, 15);

    ILI9341_WriteCommand(0x11); /* Sleep out */
    ILI9341_Deselect();
    HAL_Delay(120);
    ILI9341_Select();

    ILI9341_WriteCommand(0x29); /* Display ON */
    ILI9341_Deselect();
}

void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    ILI9341_Select();

    ILI9341_WriteCommand(0x2A); /* Column Address Set */
    ILI9341_WriteData((uint8_t[]){(uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
                                   (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF)}, 4);

    ILI9341_WriteCommand(0x2B); /* Page Address Set */
    ILI9341_WriteData((uint8_t[]){(uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
                                   (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF)}, 4);

    ILI9341_WriteCommand(0x2C); /* Memory Write */
    /* CS is left asserted: pixel data will be written immediately after. */
}

void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT) return;
    if (x + w > ILI9341_WIDTH)  w = ILI9341_WIDTH - x;
    if (y + h > ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

    ILI9341_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
    uint8_t line[64]; /* 32 pixels/chunk, small stack buffer */
    for (uint16_t i = 0; i < 32; i++) {
        line[2 * i]     = hi;
        line[2 * i + 1] = lo;
    }

    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_SET);
    uint32_t total = (uint32_t)w * (uint32_t)h;
    while (total > 0) {
        uint32_t chunk = total > 32 ? 32 : total;
        HAL_SPI_Transmit(&hspi1, line, (uint16_t)(chunk * 2), HAL_MAX_DELAY);
        total -= chunk;
    }

    ILI9341_Deselect();
}

void ILI9341_FillScreen(uint16_t color) {
    ILI9341_FillRect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT) return;
    ILI9341_SetAddressWindow(x, y, x, y);
    ILI9341_WriteData((uint8_t[]){(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)}, 2);
    ILI9341_Deselect();
}

#define ILI9341_FONT_MAX_SCALE 4

void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    if (scale < 1) scale = 1;
    if (scale > ILI9341_FONT_MAX_SCALE) scale = ILI9341_FONT_MAX_SCALE;
    if (c < 0x20 || c > 0x7E) c = ' ';
    const uint8_t *glyph = font5x7[(uint8_t)c - 0x20];

    uint16_t cell_w = 6 * scale;
    uint16_t cell_h = 8 * scale;
    if (x + cell_w > ILI9341_WIDTH || y + cell_h > ILI9341_HEIGHT) return;

    uint8_t buf[6 * ILI9341_FONT_MAX_SCALE * 8 * ILI9341_FONT_MAX_SCALE * 2];
    uint16_t idx = 0;
    for (uint16_t row = 0; row < cell_h; row++) {
        uint16_t glyph_row = row / scale;
        for (uint16_t col = 0; col < cell_w; col++) {
            uint16_t glyph_col = col / scale;
            uint16_t color = bg;
            if (glyph_row < 7 && glyph_col < 5 && (glyph[glyph_col] & (1 << glyph_row)))
                color = fg;
            buf[idx++] = (uint8_t)(color >> 8);
            buf[idx++] = (uint8_t)(color & 0xFF);
        }
    }

    ILI9341_SetAddressWindow(x, y, x + cell_w - 1, y + cell_h - 1);
    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_SET);
    HAL_SPI_Transmit(&hspi1, buf, idx, HAL_MAX_DELAY);
    ILI9341_Deselect();
}

void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale) {
    uint16_t cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += 8 * scale;
        } else {
            ILI9341_DrawChar(cx, y, *str, fg, bg, scale);
            cx += 6 * scale;
        }
        str++;
    }
}
