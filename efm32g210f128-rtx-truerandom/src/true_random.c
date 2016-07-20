
#include "true_random.h"
#include "em_cmu.h"
#include "em_adc.h"

/* ----------------------------------------- */

void trng_init(void)
{
    CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
    
    /* Enable clock for ADC0 */
    CMU_ClockEnable(cmuClock_ADC0, true);
    
    init_adc0();
}


uint8_t trng_random_bit_raw(void)
{
    return (get_adc_ch5() & 0x01);
}


uint8_t trng_random_bit_raw2(void)
{
    // Software whiten bits using Von Neumann algorithm
    //
    // von Neumann, John (1951). "Various techniques used in connection
    // with random digits". National Bureau of Standards Applied Math Series
    // 12:36.
    //
    for (;;) {
        uint8_t a = trng_random_bit_raw() | (trng_random_bit_raw() << 1);
        if (a == 1) return 0; // 1 to 0 transition: log a zero bit
        if (a == 2) return 1; // 0 to 1 transition: log a one bit
        // For other cases, try again.
    }
}


uint8_t trng_random_bit(void)
{
    // Software whiten bits using Von Neumann algorithm
    //
    // von Neumann, John (1951). "Various techniques used in connection
    // with random digits". National Bureau of Standards Applied Math Series
    // 12:36.
    //
    for (;;) {
        int a = trng_random_bit_raw2() | (trng_random_bit_raw2() << 1);
        if (a == 1) return 0; // 1 to 0 transition: log a zero bit
        if (a == 2) return 1; // 0 to 1 transition: log a one bit
        // For other cases, try again.
    }
}

uint8_t trng_random_byte(void)
{
    uint8_t result = 0;
    uint8_t i;
    for (i = 8; i--;) result += result + trng_random_bit();
    return result;
}


uint16_t trng_random16(void)
{
    uint16_t result = 0;
    uint8_t i;
    for (i = 15; i--;) result += result + trng_random_bit();
    return result;
}

uint32_t trng_random32(void)
{
    uint32_t result = 0;
    uint8_t i;
    for (i = 31; i--;) result += result + trng_random_bit();
    return result;
}

uint32_t trng_random(uint32_t howBig)
{
    uint32_t randomValue;
    // uint32_t maxRandomValue;
    uint32_t topBit;
    uint32_t bitPosition;

    if (!howBig) return 0;
    randomValue = 0;
    if (howBig & (howBig - 1)) {
        // Range is not a power of 2 - use slow method
        topBit = howBig - 1;
        topBit |= topBit >> 1;
        topBit |= topBit >> 2;
        topBit |= topBit >> 4;
        topBit |= topBit >> 8;
        topBit |= topBit >> 16;
        topBit = (topBit + 1) >> 1;

        bitPosition = topBit;
        do {
            // Generate the next bit of the result
            if (trng_random_bit()) randomValue |= bitPosition;

            // Check if bit
            if (randomValue >= howBig) {
                // Number is over the top limit - start again.
                randomValue = 0;
                bitPosition = topBit;
            }
            else {
                // Repeat for next bit
                bitPosition >>= 1;
            }
        }
        while (bitPosition);
    }
    else {
        // Special case, howBig is a power of 2
        bitPosition = howBig >> 1;
        while (bitPosition) {
            if (trng_random_bit()) randomValue |= bitPosition;
            bitPosition >>= 1;
        }
    }
    return randomValue;
}

uint32_t trng_random_range(uint32_t howSmall, uint32_t howBig)
{
    if (howSmall >= howBig) return howSmall;
    uint32_t diff = howBig - howSmall;
    return trng_random(diff) + howSmall;
}


void trng_memfill(char* location, int size)
{
    for (; size--;) *location++ = trng_random_byte();
}

void trng_mac(uint8_t* macLocation)
{
    trng_memfill((char*)macLocation, 6);
}

void trng_uuid(uint8_t* uuidLocation)
{
    // Generate a Version 4 UUID according to RFC4122
    trng_memfill((char*)uuidLocation, 16);
    // Although the UUID contains 128 bits, only 122 of those are random.
    // The other 6 bits are fixed, to indicate a version number.
    uuidLocation[8] &= 0xBF;
    uuidLocation[8] |= 0x80;

    // set version
    // must be 0b0100xxxx
    uuidLocation[6] &= 0x4F; //0b01001111
    uuidLocation[6] |= 0x40; //0b01000000
}




//================================================================================
// ADC0_enter_DefaultMode_from_RESET
//================================================================================
void init_adc0(void)
{
    // $[ADC_Init]
    ADC_Init_TypeDef init = ADC_INIT_DEFAULT;

    init.ovsRateSel = adcOvsRateSel2;
    init.lpfMode = adcLPFilterBypass;
    init.warmUpMode = adcWarmupFastBG;
    init.timebase = ADC_TimebaseCalc(0);
    init.prescale = ADC_PrescaleCalc(7000000, 0);
    init.tailgate = 0;

    ADC_Init(ADC0, &init);
    // [ADC_Init]$

    // $[ADC_InitSingle]
    ADC_InitSingle_TypeDef initsingle = ADC_INITSINGLE_DEFAULT;

    initsingle.prsSel = adcPRSSELCh0;
    initsingle.acqTime = adcAcqTime1;
    initsingle.reference = adcRefVDD; // adcRef2V5;
    initsingle.resolution = adcRes12Bit;
    initsingle.input = adcSingleInpCh5;
    initsingle.diff = 0;
    initsingle.prsEnable = 0;
    initsingle.leftAdjust = 0;
    initsingle.rep = 0;

    /* Initialize a single sample conversion.
     * To start a conversion, use ADC_Start().
     * Conversion result can be read with ADC_DataSingleGet(). */
    ADC_InitSingle(ADC0, &initsingle);
    // [ADC_InitSingle]$

    // $[ADC_InitScan]
    // [ADC_InitScan]$

}


uint32_t get_adc_ch5(void)
{
    ADC_Start(ADC0, adcStartSingle);

    /* Wait while conversion is active */
    while (ADC0->STATUS & ADC_STATUS_SINGLEACT) ;

    /* Get ADC result */
    return ADC_DataSingleGet(ADC0);
}
