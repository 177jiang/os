#ifndef __jyos_bitmap_h_
#define __jyos_bitmap_h_

#include <stdint.h>

#define bit_array_type uint8_t
#define BIT_FULL ((bit_array_type)(0xFFFFFFFF))

void set_n_bits(bit_array_type *arr, uint32_t start, uint32_t n);
void set_bit(bit_array_type *arr, uint32_t position);

void clear_n_bits(bit_array_type *arr, uint32_t start, uint32_t n);
void clear_bit(bit_array_type *arr, uint32_t position);

bit_array_type get_bit(bit_array_type *arr, uint32_t n);





#endif
