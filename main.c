
#include <stdio.h>
#include <pico/stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#define SM7080G_PWR 7

#define UART_TX_PIN 8
#define UART_RX_PIN 9

#define DATA_BITS 8
#define STOP_BITS 1

#define UART_ID uart1
#define BAUD_RATE 9600

#define PARITY UART_PARITY_NONE

char bob[256] = {'\0'};
int i = 0;

void vBlinkTask() {
   while (1) {
      gpio_put(PICO_DEFAULT_LED_PIN, 1);
      vTaskDelay(250);
      gpio_put(PICO_DEFAULT_LED_PIN, 0);
      vTaskDelay(250);
   }
}

void powerOn() {
   gpio_put(SM7080G_PWR, 1);
	vTaskDelay(1000);
	gpio_put(SM7080G_PWR, 0);
	vTaskDelay(5000);
}

void quickWrite(char * command) {
   uart_puts(UART_ID, command);
   
}

void on_uart_rx() {
   // printf("Response: !\r\n");

    while (uart_is_readable(UART_ID)) {
        char ch = uart_getc(UART_ID);
        bob[i++] = ch;
      //   if (uart_is_writable(UART_ID)) {
         // printf("%c\n", ch);
      //   }

         if (i > 255) {
            i = 0;
         }
    }
}

void gpsTestTask() {
   
   vTaskDelay(1000);

   printf("Hello World!!\r\n");
   gpio_init(SM7080G_PWR);
   gpio_set_dir(SM7080G_PWR, GPIO_OUT);

   printf("Hello World!2!\r\n");

   // Set up our UART with the required speed.
   uart_init(UART_ID, BAUD_RATE);

   printf("Hello World!3!\r\n");

   // Set the TX and RX pins by using the function select on the GPIO
   // Set datasheet for more information on function select
   gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
   gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

   printf("Hello World!4!\r\n");
     // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

   printf("Hello World!5!\r\n");

    // Set our data format
   //  uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

   printf("Hello World!6!\r\n");
    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
   //  int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

   printf("Hello World!7!\r\n");


    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART1_IRQ, on_uart_rx);

   printf("Hello World!8!\r\n");

    irq_set_enabled(UART1_IRQ, true);

   printf("Hello World!9!\r\n");


    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);

   printf("Hello World!10!\r\n");


   powerOn();
   quickWrite("AT\r\n");

   printf("Hello World!11!\r\n");

   while (1) {
	   vTaskDelay(1000);
      quickWrite("AT\r\n");
      printf("%s\r\n", bob);
   }

}

void main() {
   stdio_init_all();
   gpio_init(PICO_DEFAULT_LED_PIN);
   gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

   sleep_ms(5000);

   xTaskCreate(vBlinkTask, "Blinky Blink", 128, NULL, 1, NULL);
   xTaskCreate(gpsTestTask, "GPS Task", 128, NULL, 1, NULL);

   vTaskStartScheduler();
}