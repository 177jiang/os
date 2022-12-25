#include <datastructs/bitmap.h>
#include <stddef.h>

#define get_bit_message(position)               \
  size_t tlen = sizeof(bit_array_type);         \
  uint32_t group = position / tlen;             \
  uint32_t msk = (0x80U >> (position % tlen));

#define get_chunk_message(start, n)             \
  size_t tlen = sizeof(bit_array_type);         \
  uint32_t group = (start / tlen);              \
  uint32_t offset = (start % tlen);             \
  uint32_t group_count = (offset + n) / tlen;   \
  uint32_t remainder = (offset + n) % tlen;     \
  uint32_t shift_l =                            \
            (start + offset) < tlen ? (n) : (tlen - offset) ;





void set_n_bits(bit_array_type *arr, uint32_t start, uint32_t n){

  get_chunk_message(start, n);
  arr[group] |= ((1U << shift_l) - 1) << (tlen - offset - shift_l);

  if(group_count < 1)return ;
  ++group;
  for(uint32_t i=0; i<group_count - 1; ++i, ++group){
    arr[group] = BIT_FULL;
  }

  arr[group] |= (((1U << remainder) - 1) << (tlen - remainder));

}
void set_bit(bit_array_type *arr, uint32_t position){
  get_bit_message(position);
  arr[group] |= msk;
}

void clear_n_bits(bit_array_type *arr, uint32_t start, uint32_t n){
  get_chunk_message(start, n);
  arr[group] &= ~((1U << shift_l) - 1) << (tlen - offset - shift_l);

  if(group_count < 1)return ;
  ++group;
  for(uint32_t i=0; i<group_count - 1; ++i, ++group){
    arr[group] = 0;
  }

  arr[group] &= ~(((1U << remainder) - 1) << (tlen - remainder));
}
void clear_bit(bit_array_type *arr, uint32_t position){
  get_bit_message(position);
  arr[group] &= ~(msk);
}

bit_array_type get_bit(bit_array_type *arr, uint32_t position){
  get_bit_message(position);
  return (arr[group]  & (~msk));
}
