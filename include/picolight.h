#include "pico/stdlib.h"

int isInitPicoLightThing = 0;
const uint LED_PIN = 25;

void initPicoLightThing() {
    if (!isInitPicoLightThing) {
        isInitPicoLightThing = 1;
        gpio_init(LED_PIN);
    }
}

void light_up(bool y) {
    initPicoLightThing();
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, y ? 1 : 0);
}

int yesss = 0;

void light_up_once(int y) {
    initPicoLightThing();
    if (y == 1) {
        if (!yesss) {
            yesss = 1;
            gpio_set_dir(LED_PIN, GPIO_OUT);
            gpio_put(LED_PIN, 1);
        }
    }
}