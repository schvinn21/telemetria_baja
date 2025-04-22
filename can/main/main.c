#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "can_receiver.h"

void app_main(void) {
    can_receiver_init();
}