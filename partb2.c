
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

    // Change to 1 for Task C = 1400 ms
    #define C_LONG_TEST 1

    // Task declarations
    void TaskA(void *pvParameters);
    void TaskB(void *pvParameters);
    void TaskC(void *pvParameters);

    // Outputs for the LEDs
    DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2);

    // Turn off all LEDs
    PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2));

    // Initialize UART0
    UBRR0H = 0;
    UBRR0L = 25;
    UCSR0A = 0;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    // Configure Timer1
    // 16 MHz / 64 = 250,000 counts per second
    // 250 timer counts = 1 millisecond
    TCCR1A = 0;
    TCCR1B = (1 << CS11) | (1 << CS10);
    TCNT1 = 0;

    // Create tasks using Rate Monotonic priorities
    BaseType_t a = xTaskCreate(TaskA, "A", 256, NULL, 3, NULL);
    BaseType_t b = xTaskCreate(TaskB, "B", 256, NULL, 2, NULL);
    BaseType_t c = xTaskCreate(TaskC, "C", 256, NULL, 1, NULL);

    // Check that all tasks were created successfully
    if (a != pdPASS || b != pdPASS || c != pdPASS)
    {
        while (1);
    }

    // Start FreeRTOS scheduling
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

// Send one character through UART
void uartPrint(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

// Report when each task completes
// + means completed by deadline
// ! means deadline was missed
void printResult(char name, bool missed)
{
    uartPrint(name);
    uartPrint(missed ? '!' : '+');
    uartPrint('\n');
}

// Uses Timer1 instead of vTaskDelay()
// Interrupts remain enabled so WDT ticks continue
void busyWaitMs(uint16_t ms)
{
    while (ms > 0)
    {
        uint16_t start = TCNT1;

        // Wait for 250 Timer1 counts = about 1 ms
        while ((uint16_t)(TCNT1 - start) < 250)
        {
            // Keep CPU occupied
        }

        ms--;
    }
}

// Shared function for all three blocking tasks
void runTask(uint8_t pin, uint16_t cpuTime,
             uint16_t period, char name)
{
    // Record the initial wake time
    TickType_t lastWake = xTaskGetTickCount();

    // Convert period to FreeRTOS ticks
    TickType_t periodTicks = msToTicks(period);

    // Select the LED output pin
    uint8_t mask = (1 << pin);

    while (1)
    {
        // Calculate the deadline for this task instance
        TickType_t deadline = lastWake + periodTicks;

        // Suspend the scheduler so no other task can run
        // Interrupts continue, but task switching is paused
        vTaskSuspendAll();

        // Turn the LED ON
        PORTB |= mask;

        // Occupy the CPU for the entire execution time
        // Unlike Part B1, this does not yield the CPU
        busyWaitMs(cpuTime);

        // Turn the LED OFF
        PORTB &= ~mask;

        // Resume normal FreeRTOS scheduling
        xTaskResumeAll();

        // Check if the task completed after its deadline
        TickType_t finish = xTaskGetTickCount();
        bool missed = ((int16_t)(finish - deadline) > 0);

        // Print task name and deadline status
        printResult(name, missed);

        // Wait until the next scheduled release
        vTaskDelayUntil(&lastWake, periodTicks);
    }
}

/*--------------------Tasks-------------------------------*/

// Task A: Green LED on PB0
// Highest priority = 3
void TaskA(void *pvParameters)
{
    (void)pvParameters;

    runTask(PB0, 150, 750, 'A');
}

// Task B: Red LED on PB1
// Middle priority = 2
void TaskB(void *pvParameters)
{
    (void)pvParameters;

    runTask(PB1, 300, 1200, 'B');
}

// Task C: Blue LED on PB2
// Lowest priority = 1
void TaskC(void *pvParameters)
{
    (void)pvParameters;

#if C_LONG_TEST
    runTask(PB2, 1400, 1500, 'C');
#else
    runTask(PB2, 450, 1500, 'C');
#endif
}

