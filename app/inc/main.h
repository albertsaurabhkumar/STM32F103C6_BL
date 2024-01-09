#include "stdint.h"
#include "stdbool.h"
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/can.h>

typedef enum {
  SendMsg,
  WaitforAck,
  ReceiveMsg,
  SendAck,
  BlinkLed
}state_machin_t;

int main(void);