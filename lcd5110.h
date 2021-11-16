#ifndef LCD5110_H_
#define LCD5110_H_

#include "stdbool.h"

// platform definition
#if defined(STM32L476xx)
#include "stm32l4xx_hal.h"
#elif defined(STM32H743xx)
#include "stm32h7xx_hal.h"
#else
#error Platform not implemented
#endif

#ifndef LCD_SPI_TIMEOUT
#define LCD_SPI_TIMEOUT 1000
#endif

#define LCD_WIDTH 84
#define LCD_HEIGHT 48

typedef enum {
	LCD_COLOR_WHITE = 0, LCD_COLOR_BLACK
} LCD_color;

typedef struct {

	SPI_HandleTypeDef *spi_handle;
	GPIO_TypeDef *nsce_port;
	uint16_t nsce_pin;
	GPIO_TypeDef *dnc_port;
	uint16_t dnc_pin;
	GPIO_TypeDef *nrst_port;
	uint16_t nrst_pin;
	uint8_t buffer[LCD_WIDTH * LCD_HEIGHT / 8];
	bool update_required;
	uint8_t update_min_x;
	uint8_t update_min_y;
	uint8_t update_max_x;
	uint8_t update_max_y;

} LCD_handle;

void LCD_init(LCD_handle *handle);
void LCD_setContrast(LCD_handle *handle, uint8_t contrast);
void LCD_setBias(LCD_handle *handle, uint8_t bias);
void LCD_setInverted(LCD_handle *handle, bool inverted);
void LCD_setPixel(LCD_handle *handle, uint8_t x, uint8_t y, LCD_color color);
void LCD_update(LCD_handle *handle);

#endif /* LCD5110_H_ */
