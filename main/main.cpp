#include "VFD.hpp"
#include "freertos/task.h"


extern "C" void app_main(void) {

    // 配置参数
    VFD::Config config{
        .mosi_pin = GPIO_NUM_13,
        .clk_pin = GPIO_NUM_14,
        .cs_pin = GPIO_NUM_15,
        .rst_pin = GPIO_NUM_12,
        .en_pin = GPIO_NUM_0,
        .clock_speed = 500000,
        .display_digits = 8
    };

    // 创建VFD实例
    VFD vfd(config);
    vfd.init();

    // 自定义字模
    const VFD::Font custom_fonts[] = {
        {0, {0x04,0x02,0x04,0x08,0x30}},  // 0ヘ
        {1, {0x41,0x42,0x20,0x10,0x08}},  // 1ン
        {2, {0x41,0x22,0x10,0x08,0x07}},  // 2ソ
        {3, {0x08,0x44,0x26,0x15,0x0c}},  // 3ク
        {4, {0x14,0x14,0x7f,0x0a,0x0a}},  // 4キ
    };

    while(true) {
        // 全亮测试
        vfd.full_test(1000);

        // 显示字符串
        vfd.write_string(0, "12:34:56");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        vfd.clear();

        // 显示自定义字符
        for(auto& font : custom_fonts) {
            vfd.load_custom_font(font);
            vfd.write_char(font.id, font.id); 
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        vfd.clear();
    }
}