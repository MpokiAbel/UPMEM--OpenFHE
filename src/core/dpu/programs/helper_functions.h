#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// This is the number of elements  that is to be copied from the host

void ModAddFastEq(uint64_t* m_value, uint64_t b_m_value, uint64_t modulus_m_values) {
    *m_value += b_m_value;

    if (*m_value >= modulus_m_values)
        *m_value -= modulus_m_values;
}

/*
    Performing the ModAddEq operation make sure to cast the operations to 64 bits operations
*/
void ModAddEq(uint64_t* m_value, uint64_t* b_m_value, uint64_t modulus_m_values, size_t size) {
    
    for (size_t i = 0; i < size; i++) {
        ModAddFastEq(m_value + i, *(b_m_value + i), modulus_m_values);
    }
}