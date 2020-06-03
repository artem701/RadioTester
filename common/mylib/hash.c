
#include "hash.h"

uint8_t get_hash(uint8_t* data, size_t size)
{
  uint8_t hash = 0;
  
  for (size_t i = 0; i < size; ++i)
    hash += data[i] * 211;

  return hash;
}

void hashify(uint8_t* data, size_t size)
{
  data[size] = get_hash(data, size);
}

bool check_hash(uint8_t* data, size_t size)
{
  return get_hash(data, size - 1) == data[size - 1];
}
