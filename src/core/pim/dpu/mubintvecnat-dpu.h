#include "intnat-dpu.h"

/* We include implementations for the modular arithmetics here !!. At the moment we have the following.
    1. Elementwise Modular addition i,e modAddEq
    2. Elementwise Modular multiplication ModMulNoCheckEq which is based on the barret reduction algorithm

Other modular operation can easily be added with this interface to make the PIM backend exhausive !! example implementations not included
    1. Elementwise modular comparisons with additions, multiplications etc
    2. Elementwise modular reduction algorithms
    
*/
// Performing the ModAddEq operation make sure to cast the operations to 64 bits operations
static void ModAddEq(NativeInt* m_value, NativeInt* b_m_value, NativeInt modulus_m_values, size_t size) {
    for (size_t i = 0; i < size; i++) {
        ModAddFastEq(m_value + i, *(b_m_value + i), modulus_m_values);
    }
}

static void ModMulNoCheckEq(NativeInt* m_value, NativeInt* b_m_value, NativeInt modulus_m_values, NativeInt mu_value,
                            size_t size) {
    // NativeInt mu{m_modulus.ComputeMu()};
    for (size_t i = 0; i < size; ++i)
        ModMulFastEq(m_value + i, *(b_m_value + i), modulus_m_values, mu_value);
}