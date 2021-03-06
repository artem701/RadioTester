
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Evaluate the control sum for data[size] block
uint8_t get_hash(uint8_t* data, size_t size);

// Evaluate and add control sum value to the end of block
// Be sure element data[size] is accessable!
void hashify(uint8_t* data, size_t size);

// Evaluates hash for data[size - 1] block 
// and compares to the actual value
// returns true, if hash is ok
bool check_hash(uint8_t* data, size_t size);