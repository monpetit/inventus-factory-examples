#ifndef __TRUE_RANDOM_H
#define __TRUE_RANDOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif    

void trng_init(void);
uint8_t trng_random_bit_raw(void);
uint8_t trng_random_bit_raw2(void);
uint8_t trng_random_bit(void);
uint8_t trng_random_byte(void);
uint16_t trng_random16(void);
uint32_t trng_random32(void);
uint32_t trng_random(uint32_t howBig);
uint32_t trng_random_range(uint32_t howSmall, uint32_t howBig);
void trng_memfill(char* location, int size);
void trng_mac(uint8_t* macLocation);
void trng_uuid(uint8_t* uuidLocation);

void init_adc0(void);
uint32_t get_adc_ch5(void);
    
#ifdef __cplusplus
}
#endif    

#endif /*  __TRUE_RANDOM_H */
