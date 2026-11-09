
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////    main.c
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

#define F_CPU 16000000UL

/*--------------------Libraries---------------------------*/
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <avr/io.h>
#include <avr/sleep.h>

/* Scheduler include files. */
#include "AVR_FreeRTOS/FreeRTOS.h"
#include "AVR_FreeRTOS/task.h"
#include "AVR_FreeRTOS/queue.h"
#include "AVR_FreeRTOS/semphr.h"

/* system time include file. */
#include "time.h"

/*-----------------------------------------------------*/
int main(void)
{
    /* TODO: Create tasks and make some magic*/

    #define C_LONG_TEST 1

    // Declare the task functions
    void TaskA(void *pvParameters);
    void TaskB(void *pvParameters);
    void TaskC(void *pvParameters);

    // LED outputs
    DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2);

    // LEDs off
    PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2));

    // Initialize UART0
    UBRR0H = 0;
    UBRR0L = 25;
    UCSR0A = 0;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    // Shortest period gets highest RM 
    BaseType_t a = xTaskCreate(TaskA, "A", 256, NULL, 3, NULL);
    BaseType_t b = xTaskCreate(TaskB, "B", 256, NULL, 2, NULL);
    BaseType_t c = xTaskCreate(TaskC, "C", 256, NULL, 1, NULL);

    // Stop
    if (a != pdPASS || b != pdPASS || c != pdPASS)
    {
        while (1);
    }

    vTaskStartScheduler();

    while (1);
}

/*--------------------Helper Functions--------------------*/

// Convert milliseconds to RTOS ticks
TickType_t msToTicks(uint16_t ms)
{
    return (TickType_t)(
        ((uint32_t)ms * configTICK_RATE_HZ + 999UL) / 1000UL
    );
}

// Send one character over UART
void uartPrint(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

// Print task completion and deadline status
void printResult(char name, bool missed)
{
    uartPrint(name);
    uartPrint(missed ? '!' : '+');
    uartPrint('\n');
}

// Three LEDs
void runTask(uint8_t pin, uint16_t cpuTime,
             uint16_t period, char name)
{
    TickType_t lastWake = xTaskGetTickCount();

    TickType_t cpuTicks = msToTicks(cpuTime);
    TickType_t periodTicks = msToTicks(period);

    uint8_t mask = (1 << pin);

    while (1)
    {
        // Calculate the deadline for this cycle
        TickType_t deadline = lastWake + periodTicks;

        // Turn on this task's LED
        PORTB |= mask;

        // Task sleeps while LED remains on
        // Other tasks can run during this delay
        vTaskDelay(cpuTicks);

        // Turn LED off after execution interval
        PORTB &= ~mask;

        // Check whether completion was late
        TickType_t finish = xTaskGetTickCount();
        bool missed = ((int16_t)(finish - deadline) > 0);

        printResult(name, missed);

        // Wait until the next periodic release
        vTaskDelayUntil(&lastWake, periodTicks);
    }
}

/*--------------------Tasks-------------------------------*/

// Task A: 150 ms ON, period 750 ms
void TaskA(void *pvParameters)
{
    (void)pvParameters;

    runTask(PB0, 150, 750, 'A');
}

// Task B: 300 ms ON, period 1200 ms
void TaskB(void *pvParameters)
{
    (void)pvParameters;

    runTask(PB1, 300, 1200, 'B');
}

// Task C: 450 ms ON, period 1500 ms
void TaskC(void *pvParameters)
{
    (void)pvParameters;

#if C_LONG_TEST
    runTask(PB2, 1400, 1500, 'C');
#else
    runTask(PB2, 450, 1500, 'C');
#endif
}
