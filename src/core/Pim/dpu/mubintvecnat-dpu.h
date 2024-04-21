#include "intnat-dpu.h"


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