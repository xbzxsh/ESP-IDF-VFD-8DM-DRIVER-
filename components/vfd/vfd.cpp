#include "VFD.hpp"
#include "esp_log.h"
#include "freertos/task.h"
// #include "rom/ets_sys.h"

static const char* TAG = "VFD";

VFD::VFD(const Config& config) : config_(config) {}

VFD::~VFD() {
    if(spi_) {
        spi_bus_remove_device(spi_);
    }
}

void VFD::init() {
    if(initialized_) return;

    // GPIO初始化
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config_.mosi_pin) |
                        (1ULL << config_.clk_pin) |
                        (1ULL << config_.cs_pin) |
                        (1ULL << config_.rst_pin) |
                        (1ULL << config_.en_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // SPI初始化
    spi_bus_config_t buscfg = {
        .mosi_io_num = config_.mosi_pin,
        .miso_io_num = -1,
        .sclk_io_num = config_.clk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        // .max_transfer_sz = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .mode = 0,
        .clock_speed_hz = config_.clock_speed,
        .spics_io_num = -1,
        .flags = SPI_DEVICE_HALFDUPLEX|SPI_DEVICE_TXBIT_LSBFIRST,
        .queue_size = 7
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_));

    // 使能VFD
    gpio_set_level(config_.en_pin, 1);
    
    // 硬件复位
    reset_sequence();

    // VFD初始化命令
    send_command(0xe0, config_.display_digits - 1);
    set_brightness(0xFF);                   // 最大亮度
    
    initialized_ = true;
    ESP_LOGI(TAG, "Initialization complete");
}

void VFD::reset_sequence() {
    // ets_delay_us(100);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(config_.rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    // ets_delay_us(100);
    gpio_set_level(config_.rst_pin, 1);
    //ets_delay_us(5);
}

void VFD::spi_transfer(uint8_t data) {
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .rx_buffer = NULL,     // 不需要接收数据
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi_, &t));
    //ets_delay_us(5);
}

void VFD::send_command(uint8_t cmd) {
    gpio_set_level(config_.cs_pin, 0);
    //ets_delay_us(5);
    spi_transfer(cmd);
    //ets_delay_us(5);
    gpio_set_level(config_.cs_pin, 1);
    //ets_delay_us(5);
}

void VFD::send_command(uint8_t cmd ,uint8_t data) {
    gpio_set_level(config_.cs_pin, 0);
    //ets_delay_us(5);
    spi_transfer(cmd);
    //ets_delay_us(5);
    spi_transfer(data);
    //ets_delay_us(5); 
    gpio_set_level(config_.cs_pin, 1);
    //ets_delay_us(5);
}


void VFD::send_command(uint8_t cmd, const uint8_t* data, size_t len) {
    gpio_set_level(config_.cs_pin, 0);  // 拉低片选信号
    //ets_delay_us(5);  // 稍微延迟一下，确保命令完全传输
    spi_transfer(cmd);  // 发送命令
    //ets_delay_us(5);  // 稍微延迟一下，确保命令完全传输
    for (uint8_t i = 0; i < len; i++) {
        if(data[i] == 0) {
            break;
        }
        spi_transfer(data[i]);  // 发送数据
        //ets_delay_us(5);  // 稍微延迟一下，确保命令完全传输
    }
    gpio_set_level(config_.cs_pin, 1);  // 拉高片选信号
    //ets_delay_us(5);  // 稍微延迟一下，确保命令完全传输
}

void VFD::write_char(uint8_t position, uint8_t char_code) {
    //0x20代表DCRAM写命令，position代表第几位数码管用于显示
    //0x00-0x07代表CGRAM存储的自定义字模数据地址，0x30-0x3F代表CGROM存储的标准字模数据地址
    send_command(0x20 + position, char_code); 
}

void VFD::write_string(uint8_t start_pos, const std::string& str) {
    //同write_char，只是可以写入多个字符,第二个以后的字符不需指定位置，直接传输数据即可
    send_command(0x20 + start_pos , reinterpret_cast<const uint8_t*>(str.c_str()), str.size());
}

void VFD::load_custom_font(const Font& font) {
    // 写入CGRAM
    send_command(0x40 + font.id, font.data, sizeof(font.data));
}

void VFD::set_brightness(uint8_t level) {
    send_command(0xE4,level);
}

void VFD::full_test(uint32_t duration_ms) {
    send_command(0xE9);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    send_command(0xE8);  // 恢复正常显示
}

void VFD::clear() {
    std::string empty(config_.display_digits, ' ');
    write_string(0, empty);
}