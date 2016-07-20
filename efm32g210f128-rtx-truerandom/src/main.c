#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_int.h"
#include "em_usart.h"

#include "true_random.h"
#include "cmsis_os.h"

#define LED_PIN             (0)
#define LED_PORT            (gpioPortA)

#define REPEAT_COUNT    100
#define DELAY_SHORT     50
#define DELAY_LONG      2000

void reset_blink(volatile uint8_t countdown);

void init_clocks(void);
void init_usart1(void);
void init_portio(void);
void print_uuid(uint8_t *buffer, bool crlf);


void BlinkLedThread(void const *args)
{
    while (1) {
        GPIO_PinOutToggle(LED_PORT, LED_PIN);
        osDelay(1000);
    }
}

/* Thread definition */
osThreadDef(BlinkLedThread, osPriorityNormal, 1, 0);

uint32_t adc_result;
uint8_t uuid_buff[16];

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{   
    /* Chip errata */
    CHIP_Init();

    /* Enter default mode */
    init_clocks();
    init_usart1();
    init_portio();
    trng_init();

    /* Ensure core frequency has been updated */
    SystemCoreClockUpdate();

    reset_blink(30);
    printf("\r\n*** SYSTEM START ***\r\n\r\n");
    
    /* create thread 1 */
    osThreadCreate(osThread(BlinkLedThread), NULL);

    /* Infinite loop */
    volatile uint32_t loop_count = REPEAT_COUNT;
    while (loop_count--) {
        adc_result = get_adc_ch5();
        printf("adc0 [ch5] %u\t%u\r\n", adc_result, (adc_result & 0x01));
        osDelay(DELAY_SHORT);
    }
    printf("\r\n\r\n");
    osDelay(DELAY_LONG);

    loop_count = REPEAT_COUNT;
    while (loop_count--) {
        printf("[random byte] %u\r\n", trng_random_byte());
        osDelay(DELAY_SHORT);
    }
    printf("\r\n\r\n");
    osDelay(DELAY_LONG);

    loop_count = REPEAT_COUNT;
    while (loop_count--) {
        printf("[random 16bit] %u\r\n", trng_random16());
        osDelay(DELAY_SHORT);
    }
    printf("\r\n\r\n");
    osDelay(DELAY_LONG);

    loop_count = REPEAT_COUNT;
    while (loop_count--) {
        printf("[random 32bit] %u\r\n", trng_random32());
        osDelay(DELAY_SHORT);
    }
    printf("\r\n\r\n");
    osDelay(2000);

    loop_count = REPEAT_COUNT;
    while (loop_count--) {
        printf("[random range 100-999] %u\r\n", trng_random_range(100, 1000));
        osDelay(DELAY_SHORT);
    }
    printf("\r\n\r\n");
    osDelay(DELAY_LONG);

    while (1) {
        trng_uuid(uuid_buff);
        printf("[uuid] ");
        print_uuid(uuid_buff, true);
        osDelay(DELAY_SHORT);
    }
}


void reset_blink(volatile uint8_t countdown)
{
    while (countdown--) {
        GPIO_PinOutToggle(LED_PORT, LED_PIN);
        osDelay(DELAY_SHORT / 2);
    }
    GPIO_PinOutSet(LED_PORT, LED_PIN); // LED OFF
    osDelay(DELAY_LONG);
}


void init_clocks(void)
{
    // $[HFXO]
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_HFXOMODE_MASK) | CMU_CTRL_HFXOMODE_XTAL;

    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_HFXOBOOST_MASK)
                | CMU_CTRL_HFXOBOOST_50PCENT;

    SystemHFXOClockSet(32000000);

    // $[Use oscillator source]
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_LFXOMODE_MASK) | CMU_CTRL_LFXOMODE_XTAL;
    // [Use oscillator source]$

    // $[LFXO Boost Percent]
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_LFXOBOOST_MASK)
                | CMU_CTRL_LFXOBOOST_100PCENT;
    // [LFXO Boost Percent]$

    // $[LFXO enable]
    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
    // [LFXO enable]$

    // $[HFXO enable]
    CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
    // [HFXO enable]$

    // $[High Frequency Clock select]
    /* Using HFXO as high frequency clock, HFCLK */
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

    /* Enable peripheral clock */
    CMU_ClockEnable(cmuClock_HFPER, true);

    // [High Frequency Clock select]$

    // $[LF clock tree setup]
    /* Enable LF clocks */
    CMU_ClockEnable(cmuClock_CORELE, true);
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
    // [LF clock tree setup]$

    // $[Peripheral Clock enables]

    /* Enable clock for USART1 */
    CMU_ClockEnable(cmuClock_USART1, true);

    /* Enable clock for GPIO by default */
    CMU_ClockEnable(cmuClock_GPIO, true);

    // [Peripheral Clock enables]$
}


//================================================================================
// USART1_enter_DefaultMode_from_RESET
//================================================================================
void init_usart1(void)
{
    // $[USART_InitAsync]
    USART_InitAsync_TypeDef initasync = USART_INITASYNC_DEFAULT;

    initasync.baudrate = 115200;
    initasync.databits = usartDatabits8;
    initasync.parity = usartNoParity;
    initasync.stopbits = usartStopbits1;
    initasync.oversampling = usartOVS16;
#if defined( USART_INPUT_RXPRS ) && defined( USART_CTRL_MVDIS )
    initasync.mvdis = 0;
    initasync.prsRxEnable = 0;
    initasync.prsRxCh = 0;
#endif

    USART_InitAsync(USART1, &initasync);
    // [USART_InitAsync]$
}


void init_portio(void)
{
    /* Pin PA0 is configured to Push-pull */
    GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE0_MASK)
                       | GPIO_P_MODEL_MODE0_PUSHPULL;

    /* Pin PA1 is configured to Input enabled with pull-up */
    GPIO->P[0].DOUT |= (1 << 1);
    GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE1_MASK)
                       | GPIO_P_MODEL_MODE1_INPUTPULL;

    // $[Port C Configuration]

    /* Pin PC0 is configured to Push-pull */
    GPIO->P[2].MODEL = (GPIO->P[2].MODEL & ~_GPIO_P_MODEL_MODE0_MASK)
                       | GPIO_P_MODEL_MODE0_PUSHPULL;

    /* Pin PC1 is configured to Input enabled */
    GPIO->P[2].MODEL = (GPIO->P[2].MODEL & ~_GPIO_P_MODEL_MODE1_MASK)
                       | GPIO_P_MODEL_MODE1_INPUT;
    // [Port C Configuration]$

    // $[Route Configuration]
    /* Enable signals RX, TX */
    USART1->ROUTE |= USART_ROUTE_RXPEN | USART_ROUTE_TXPEN;
    // [Route Configuration]$
}






void print_uuid(uint8_t *buffer, bool crlf)
{
    static char hexbar[] = "0123456789abcdef";
    for (int i = 0; i < 16; i++) {
        uint8_t ch = *buffer++;
        putchar(*(hexbar + (ch >> 4)));
        putchar(*(hexbar + (ch & 0x0f)));

        if ((i == 3) || (i == 5) || (i == 7) || (i == 9))
            putchar('-');
    }
    if (crlf) {
        putchar('\r');
        putchar('\n');
    }
}

