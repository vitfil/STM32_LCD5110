#include "lcd5110.h"

#include <assert.h>
#include <string.h>

#define LCD_SET_BIT(BYTE, BIT)      ((BYTE) |= (uint8_t)(1u << (uint8_t)(BIT)))
#define LCD_CLEAR_BIT(BYTE, BIT)    ((BYTE) &= (uint8_t)(~(uint8_t)(1u << (uint8_t)(BIT))))
#define LCD_MIN(VAL1, VAL2)         (((VAL1)<(VAL2))?(VAL1):(VAL2))
#define LCD_MAX(VAL1, VAL2)         (((VAL1)>(VAL2))?(VAL1):(VAL2))

#define LCD_EXTENDED_INSTRUCTION    ((uint8_t)0x01)
#define LCD_DISPLAYNORMAL   		((uint8_t)0x04)
#define LCD_DISPLAYINVERTED 		((uint8_t)0x05)
#define LCD_DISPLAYCONTROL  		((uint8_t)0x08)
#define LCD_SETBIAS         		((uint8_t)0x10)
#define LCD_FUNCTIONSET     		((uint8_t)0x20)
#define LCD_SETYADDR        		((uint8_t)0x40)
#define LCD_SETXADDR        		((uint8_t)0x80)
#define LCD_SETVOP          		((uint8_t)0x80)

// private functions
static void LCD_write(LCD_handle *handle, uint8_t *values, uint16_t size) {
	HAL_GPIO_WritePin(handle->nsce_port, handle->nsce_pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(handle->spi_handle, values, size, LCD_SPI_TIMEOUT);
	HAL_GPIO_WritePin(handle->nsce_port, handle->nsce_pin, GPIO_PIN_SET);
}

static void LCD_data(LCD_handle *handle, uint8_t *data, uint16_t size) {
	HAL_GPIO_WritePin(handle->dnc_port, handle->dnc_pin, GPIO_PIN_SET);
	LCD_write(handle, data, size);
}

static void LCD_command(LCD_handle *handle, uint8_t command) {
	HAL_GPIO_WritePin(handle->dnc_port, handle->dnc_pin, GPIO_PIN_RESET);
	LCD_write(handle, &command, 1);
}

// public functions
void LCD_init(LCD_handle *handle) {
	assert(handle->spi_handle != NULL);
	assert(handle->dnc_port != NULL);
	assert(handle->nrst_port != NULL);
	assert(handle->nsce_port != NULL);

	// Initially unselect pin.
	HAL_GPIO_WritePin(handle->nsce_port, handle->nsce_pin, GPIO_PIN_SET);

	// Perform hardware reset.
	HAL_GPIO_WritePin(handle->nrst_port, handle->nrst_pin, GPIO_PIN_RESET);
	HAL_Delay(500);
	HAL_GPIO_WritePin(handle->nrst_port, handle->nrst_pin, GPIO_PIN_SET);

	// Set bias and contrast to default values.
	LCD_setBias(handle, 4);
	LCD_setContrast(handle, 50);

	// Set display to normal mode.
	LCD_setInverted(handle, true);

	// Clear the back buffer and initialize update variables.
	handle->update_required = true;
	handle->update_min_x = 0;
	handle->update_min_y = 0;
	handle->update_max_x = LCD_WIDTH - 1;
	handle->update_max_y = LCD_HEIGHT - 1;

	LCD_update(handle);
}

void LCD_setContrast(LCD_handle *handle, uint8_t contrast) {
	if (contrast > 0x7f) contrast = 0x7f;

	LCD_command(handle, LCD_FUNCTIONSET | LCD_EXTENDED_INSTRUCTION);
	LCD_command(handle, LCD_SETVOP | contrast);
	LCD_command(handle, LCD_FUNCTIONSET);
}

void LCD_setBias(LCD_handle *handle, uint8_t bias) {
	if (bias > 0x07) bias = 0x07;

	LCD_command(handle, LCD_FUNCTIONSET | LCD_EXTENDED_INSTRUCTION);
	LCD_command(handle, LCD_SETBIAS | bias);
	LCD_command(handle, LCD_FUNCTIONSET);
}

void LCD_setInverted(LCD_handle *handle, bool inverted) {
	if (inverted) LCD_command(handle, LCD_DISPLAYCONTROL | LCD_DISPLAYNORMAL);
	else LCD_command(handle, LCD_DISPLAYCONTROL | LCD_DISPLAYINVERTED);
}

void LCD_setPixel(LCD_handle *handle, uint8_t x, uint8_t y, LCD_color color) {
	if (x > LCD_WIDTH) x = LCD_WIDTH;
	if (y > LCD_HEIGHT) y = LCD_HEIGHT;

	if (color == LCD_COLOR_BLACK) LCD_SET_BIT(handle->buffer[x + (y / 8) * LCD_WIDTH], y % 8);
	else LCD_CLEAR_BIT(handle->buffer[x + (y / 8) * LCD_WIDTH], y % 8);

	// If no update was previously required, we just need to update the single pixel.
	if (!handle->update_required) {
		handle->update_min_x = handle->update_max_x = x;
		handle->update_min_y = handle->update_max_y = y;
		handle->update_required = true;

		// Otherwise resize the update bounding box accordingly.
	} else {
		handle->update_min_x = LCD_MIN(x, handle->update_min_x);
		handle->update_min_y = LCD_MIN(y, handle->update_min_y);
		handle->update_max_x = LCD_MAX(x, handle->update_max_x);
		handle->update_max_y = LCD_MAX(y, handle->update_max_y);
	}
}

void LCD_update(LCD_handle *handle) {
	// Firstly, check if we need to update at all.
	if (!handle->update_required)  return;

	uint8_t min_page = handle->update_min_y / 8;
	uint8_t max_page = handle->update_max_y / 8;

	for (uint8_t page = min_page; page <= max_page; page++) {

		uint8_t min_column = handle->update_min_x;
		uint8_t max_column = handle->update_max_x;

		LCD_command(handle, LCD_SETYADDR | page);
		LCD_command(handle, LCD_SETXADDR | min_column);

		LCD_data(handle, &handle->buffer[LCD_WIDTH * page + min_column], (max_column - min_column) + 1);
		LCD_command(handle, LCD_SETYADDR);
	}

	handle->update_required = false;
}
