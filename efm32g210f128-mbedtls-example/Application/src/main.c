#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_int.h"
#include "em_usart.h"
#include "em_adc.h"
#include "em_aes.h"

#include "timestamp.h"

#include "mbedtls/config.h"
#include "mbedtls/sha1.h"
#include "mbedtls/aes.h"

#define LED_PIN (0)
#define LED_PORT (gpioPortA)

#define USE_HARDWARE_AES 0

volatile uint32_t cyc_enc, cyc_dec;
volatile uint32_t systick_count = 0;
uint32_t clock_freq;

__STATIC_INLINE void __delay_tick(volatile uint32_t n);
__STATIC_INLINE uint32_t get_systick(void);
void __delay_ms(uint32_t milliseconds);
void setup_systick_timer(void);
void reset_blink(volatile uint8_t countdown);

void init_clocks(void);
void init_usart1(void);
void init_portio(void);

char *sha1_hexdigest(uint8_t *buffer);

/////////////////////////////////////////////////////////////////////////////////

/*
 * Simulate the MySQL aes_encrypt function
 *
 * Warning: The elusive padding bug has been implemented here as well
 * for compatibility reasons.
 *
 * @param str           A zero-terminated text string to be encrypted
 * @param pwd           A zero-terminated text password
 * @param output_length The length of the output buffer
 * @param output        The buffer that will receive the text result (hexified)
 */
int mysql_aes_encrypt(const unsigned char *str,
                      const unsigned char *pwd,
                      int output_length,
                      unsigned char *output)
{
    int i, len;
    unsigned char key[16];
    unsigned char temp[16];
    const unsigned char *p_i = str;
    unsigned char *p_o = output;
    mbedtls_aes_context ctx;

    len = strlen((const char *)str);

    // Check if there is enough space in the output buffer
    //
    if (32 + (len / 16) * 32 >= output_length)
        return -1;
    memset(output, 0, output_length);

    // Set-up the MySQL key
    //
    memset(key, 0, 16);
    for (i = 0; i < strlen((const char *)pwd); ++i)
        key[i % 16] ^= pwd[i];
#if (USE_HARDWARE_AES == 0)
    mbedtls_aes_setkey_enc(&ctx, key, 128);
#endif
#if 0
    printf("\r\n**enc KEY = [0x");
    for (i = 0; i < 16; i++) {
        printf("%02x", key[i]);
    }
    printf("]\r\n");
#endif

    timestamp_counter_reset();
    // Run through data and encrypt and stringify
    //
    while (len >= 0) {
        int use_len = 16;
        if (len < 16)
            use_len = len;

        // Set up data including padding
        //
        memcpy(temp, p_i, use_len);
        memset(temp + use_len, 16 - use_len, 16 - use_len);

#if (USE_HARDWARE_AES == 0)
        mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, temp, temp);
#else
        AES_ECB128(temp, temp, 16, key, true);
#endif
        for (i = 0; i < 16; ++i)
            sprintf((char *)p_o + i * 2, "%02X", temp[i]);

        len -= 16;
        p_i += 16;
        p_o += 32;
    }
    cyc_enc += timestamp_counter_get();

    return (0);
}

/*
 * Simulate the MySQL aes_decrypt function
 *
 * Warning: The elusive padding bug has been implemented here as well
 * for compatibility reasons.
 *
 * @param str           A zero-terminated text string to be decrypted
 * @param pwd           A zero-terminated text password
 * @param output_length The length of the output buffer
 * @param output        The buffer that will receive the text result (hexified)
 */
int mysql_aes_decrypt(const unsigned char *str,
                      const unsigned char *pwd,
                      int output_length,
                      unsigned char *output)
{
    int i, len;
    unsigned char key[16];
    unsigned char temp[16];
    const unsigned char *p_i = str;
    unsigned char *p_o = output;
    mbedtls_aes_context ctx;

    len = strlen((const char *)str);

    // Check if input is multiple of 32
    //
    if (len % 32)
        return -1;

    // Check if there is enough space in the output buffer
    //
    if ((len / 32) * 16 + 1 >= output_length)
        return -1;
    memset(output, 0, output_length);

    // Set-up the MySQL key
    //
    memset(key, 0, 16);
    for (i = 0; i < strlen((const char *)pwd); ++i)
        key[i % 16] ^= pwd[i];
#if (USE_HARDWARE_AES == 0)
    mbedtls_aes_setkey_dec(&ctx, key, 128);
#else
    uint8_t deckey[16];
    AES_DecryptKey128(deckey, key);
#endif

#if 0
    printf("\r\n**dec KEY = [0x");
    for (i = 0; i < 16; i++) {
        printf("%02x", key[i]);
    }
    printf("]\r\n");
#endif

    timestamp_counter_reset();
    // Run through data and encrypt and stringify
    //
    while (len > 0) {
        // Translate hexified data to binary value
        //
        for (i = 0; i < 32; i += 2) {
            unsigned char c, h, l;

            c = p_i[i];
            if (c >= '0' && c <= '9')
                h = c - '0';
            else if (c >= 'A' && c <= 'Z')
                h = c - 'A' + 10;
            else if (c >= 'a' && c <= 'z')
                h = c - 'a' + 10;
            else
                return -1;

            c = p_i[i + 1];
            if (c >= '0' && c <= '9')
                l = c - '0';
            else if (c >= 'A' && c <= 'Z')
                l = c - 'A' + 10;
            else if (c >= 'a' && c <= 'z')
                l = c - 'a' + 10;
            else
                return -1;

            temp[i / 2] = h * 16 + l;
        }
#if (USE_HARDWARE_AES == 0)
        mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, temp, p_o);
#else
        AES_ECB128(p_o, temp, 16, deckey, false);
#endif
        len -= 32;
        p_i += 32;
        p_o += 16;
    }
    cyc_dec += timestamp_counter_get();

    return (0);
}

const char *mysql_aes_pt[6] = {
    "0123456789abcdef",
    "0123456789abcdef",
    "0123456789abcdef0123456789abcdef",
    "0123456789abcde",
    "0123456789abcd",
    ""
};

const char *mysql_aes_pwd[6] = {
    "",
    "asd",
    "asdasdasdasdasdasdasdasdasd",
    "",
    "",
    ""
};

const char *mysql_aes_ct[6] = {
    "14F5FE746966F292651C2288BBFF46090143DB63EE66B0CDFF9F69917680151E",
    "9C0ADE6D942C5174D36C1E3D5B6483CFDA61AB008A954921B9693DE7076FB0DA",
    "F4642D42E09D157950C9A0A23049CE64F4642D42E09D157950C9A0A23049CE64BAF90C4409B3F0E4C3486787B37C0540",
    "AE769E8822731C2A1012A8C4E73FB536",
    "ACFD9247027814986F8F8E07927B9E18",
    "0143DB63EE66B0CDFF9F69917680151E"
};

int aes_cryto_test(void)
{
    unsigned char output[100];
    int i;

    memset(output, 0, 100);

    for (i = 0; i < 6; ++i) {
        printf("Encrypt Case %d: ", i);
        if (mysql_aes_encrypt((const unsigned char *)mysql_aes_pt[i], (const unsigned char *)mysql_aes_pwd[i], 100, output) != 0) {
            printf(" failed\n");
            continue;
        }
        else if (strncmp(mysql_aes_ct[i], (const char *)output, strlen(mysql_aes_ct[i]))) {
            printf(" failed\n");
            continue;
        }

        printf(" success\n");
        printf("%s\n%s\n", mysql_aes_ct[i], output);

        printf("Decrypt Case %d: ", i);
        if (mysql_aes_decrypt((const unsigned char *)mysql_aes_ct[i], (const unsigned char *)mysql_aes_pwd[i], 100, output) != 0) {
            printf(" failed\n");
            continue;
        }
        else if (strncmp(mysql_aes_pt[i], (const char *)output, strlen(mysql_aes_pt[i]))) {
            printf(" failed\n");
            continue;
        }

        printf(" success\n");
        printf("%s\n%s\n", mysql_aes_pt[i], output);
    }

    return (0);
}
/////////////////////////////////////////////////////////////////////////////////

/**************************************************************************/ /**
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

    /* Ensure core frequency has been updated */
    SystemCoreClockUpdate();

    /* Start Systick Timer */
    setup_systick_timer();

    reset_blink(30);
    printf("\r\n*** SYSTEM START ***\r\n\r\n");

    timestamp_counter_enable();
    __delay_ms(2000);

    while (1) {
        const uint8_t buff[] = "hello WORLD!";
        uint8_t outbuff[20];
        mbedtls_sha1(buff, sizeof(buff) - 1, outbuff);
        printf("sha1 hex digest = [%s]\r\n", sha1_hexdigest(outbuff));

        GPIO_PinOutToggle(LED_PORT, LED_PIN);
        uint32_t __tick = get_systick();
        printf("systick = %u\r\n", __tick);

        cyc_enc = cyc_dec = 0;
        aes_cryto_test();
        printf("*** TIMESTAMP = enc [%u], dec [%u]\r\n", cyc_enc, cyc_dec);

#if 0
        uint8_t ibuf[16] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0xde, 0x0f
        };

        uint8_t obuf[16];
        uint8_t oobuf[16];
        const uint8_t __key[16] = {
            0x3e, 0x0a, 0x0a, 0x0b, 0x04, 0x95, 0x06, 0x07,
            0xa8, 0xf9, 0xba, 0x0b, 0x61, 0x02, 0x03, 0xff
        };

#if 1
        AES_ECB128(obuf, ibuf, 16, __key, true);
#else
        mbedtls_aes_context _ctx;
        mbedtls_aes_setkey_enc(&_ctx, __key, 128);
        mbedtls_aes_crypt_ecb(&_ctx, MBEDTLS_AES_ENCRYPT, ibuf, obuf);
#endif
        printf("======\r\n");
        for (int i = 0; i < 16; i++) {
            printf("%02x ", obuf[i]);
        }
        printf("\r\n");
        printf("======\r\n");
#if 1
        uint8_t deckey[16];
        AES_DecryptKey128(deckey, __key);
        AES_ECB128(oobuf, obuf, 16, deckey, false);
#else
        mbedtls_aes_context ctx;
        mbedtls_aes_setkey_dec(&ctx, __key, 128);
        mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, obuf, oobuf);
#endif
        for (int i = 0; i < 16; i++) {
            printf("%02x ", oobuf[i]);
        }
        printf("\r\n");
#endif

        __delay_ms(3000 - (__tick % 3000));
    }
}

void SysTick_Handler(void)
{
    systick_count++;
}

__STATIC_INLINE uint32_t get_systick(void)
{
    return systick_count;
}

/******************************************************************************
 * @brief Delay function
 *****************************************************************************/
void __delay_ms(uint32_t milliseconds)
{
    uint32_t start_tick = get_systick();
    while ((get_systick() - start_tick) < milliseconds) {
    }
}

void setup_systick_timer(void)
{
    clock_freq = CMU_ClockFreqGet(cmuClock_CORE);
    /* Enable SysTick interrupt, necessary for __delay_ms() */
    if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000))
        while (1)
            ;
    NVIC_EnableIRQ(SysTick_IRQn);
}

void reset_blink(volatile uint8_t countdown)
{
    while (countdown--) {
        GPIO_PinOutToggle(LED_PORT, LED_PIN);
        __delay_ms(15);
    }
    GPIO_PinOutSet(LED_PORT, LED_PIN); // LED OFF
    __delay_ms(100);
}

void init_clocks(void)
{
    // $[HFXO]
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_HFXOMODE_MASK) | CMU_CTRL_HFXOMODE_XTAL;

    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_HFXOBOOST_MASK) | CMU_CTRL_HFXOBOOST_50PCENT;

    SystemHFXOClockSet(32000000);

    // $[Use oscillator source]
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_LFXOMODE_MASK) | CMU_CTRL_LFXOMODE_XTAL;
    // [Use oscillator source]$

    // $[LFXO Boost Percent]
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_LFXOBOOST_MASK) | CMU_CTRL_LFXOBOOST_100PCENT;
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

    // $[Peripheral Clock enables]

    /* Enable clock for ADC0 */
    CMU_ClockEnable(cmuClock_ADC0, true);

    /* Enable clock for USART1 */
    CMU_ClockEnable(cmuClock_USART1, true);

    /* Enable clock for GPIO by default */
    CMU_ClockEnable(cmuClock_GPIO, true);

    /* Enable AES clock */
    CMU_ClockEnable(cmuClock_AES, true);

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
#if defined(USART_INPUT_RXPRS) && defined(USART_CTRL_MVDIS)
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
    GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE0_MASK) | GPIO_P_MODEL_MODE0_PUSHPULL;

    /* Pin PA1 is configured to Input enabled with pull-up */
    GPIO->P[0].DOUT |= (1 << 1);
    GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE1_MASK) | GPIO_P_MODEL_MODE1_INPUTPULL;

    // $[Port C Configuration]

    /* Pin PC0 is configured to Push-pull */
    GPIO->P[2].MODEL = (GPIO->P[2].MODEL & ~_GPIO_P_MODEL_MODE0_MASK) | GPIO_P_MODEL_MODE0_PUSHPULL;

    /* Pin PC1 is configured to Input enabled */
    GPIO->P[2].MODEL = (GPIO->P[2].MODEL & ~_GPIO_P_MODEL_MODE1_MASK) | GPIO_P_MODEL_MODE1_INPUT;
    // [Port C Configuration]$

    // $[Route Configuration]
    /* Enable signals RX, TX */
    USART1->ROUTE |= USART_ROUTE_RXPEN | USART_ROUTE_TXPEN;
    // [Route Configuration]$
}

char *sha1_hexdigest(uint8_t *buffer)
{
    static char hexbar[] = "0123456789abcdef";
    static char hexbuffer[41];

    for (int i = 0; i < 20; i++) {
        hexbuffer[i * 2] = *(hexbar + (buffer[i] >> 4));
        hexbuffer[i * 2 + 1] = *(hexbar + (buffer[i] & 0x0f));
    }
    hexbuffer[40] = 0;
    return hexbuffer;
}