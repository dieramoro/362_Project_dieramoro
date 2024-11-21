#include "checkstick.h"
extern uint16_t * msg;
extern char font[];

int read_buttons()
{
    return (~GPIOC->IDR) & 0x8; // reads our 3 input buttons
    // right now have it as PC0,1,2
}

void togglexn(GPIO_TypeDef *port, int n) {

  // if (n < 0 || n > 15) {
  //       return;
  // } //handle if not 0-15 for GPIO

    // Toggle the pin using XOR operation
    port->ODR ^= (1 << n);
}
