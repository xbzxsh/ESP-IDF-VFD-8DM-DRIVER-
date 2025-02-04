#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "driver/gpio.h"
#include "driver/spi_master.h"

class VFD {
public:
    // 配置结构体
    struct Config {
        gpio_num_t mosi_pin = GPIO_NUM_13;
        gpio_num_t clk_pin = GPIO_NUM_14;
        gpio_num_t cs_pin = GPIO_NUM_15;
        gpio_num_t rst_pin = GPIO_NUM_12;
        gpio_num_t en_pin = GPIO_NUM_0;
        int clock_speed = 500000;  // 500 kHz
        uint8_t display_digits = 8;     // 默认8位数码管
    };

    // 字模数据
    struct Font {
        uint8_t id;
        uint8_t data[5];
    };

    explicit VFD(const Config& config);
    ~VFD();

    void init();
    void clear();
    void full_test(uint32_t duration_ms = 1000);
    void set_brightness(uint8_t level);
    
    void write_char(uint8_t position, uint8_t char_code);
    void write_string(uint8_t start_pos, const std::string& str);
    void load_custom_font(const Font& font);

private:
    Config config_;
    spi_device_handle_t spi_ = nullptr;
    bool initialized_ = false;
    
    void reset_sequence();
    void send_command(uint8_t cmd);
    void send_command(uint8_t cmd ,uint8_t data);
    void send_command(uint8_t cmd ,const uint8_t* data, size_t len);
    void spi_transfer(uint8_t data);
};