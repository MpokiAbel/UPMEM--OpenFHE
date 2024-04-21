#include "intnat-dpu.h"

// Initial step would be to run the individual tower switchformat in the DPU and check if correctness is achieved

template <typename VecType>
void DCRTPolyImpl<VecType>::SwitchFormat() {
    m_format = (m_format == Format::COEFFICIENT) ? Format::EVALUATION : Format::COEFFICIENT;
    size_t size{m_vectors.size()};
#pragma omp parallel for num_threads(OpenFHEParallelControls.GetThreadLimit(size))
    for (size_t i = 0; i < size; ++i)
        m_vectors[i].SwitchFormat();
}

template <typename VecType>
void PolyImpl<VecType>::SwitchFormat() {
    const auto& co{m_params->GetCyclotomicOrder()};
    const auto& rd{m_params->GetRingDimension()};
    const auto& ru{m_params->GetRootOfUnity()};

    if (rd != (co >> 1)) {
        PolyImpl<VecType>::ArbitrarySwitchFormat();
        return;
    }

    if (!m_values)
        OPENFHE_THROW(not_available_error, "Poly switch format to empty values");

    if (m_format != Format::COEFFICIENT) {
        m_format = Format::COEFFICIENT;
        CRTFFTInverseTransformFromBitReverseInPlace(ru, co, &(*m_values));
        return;
    }
    m_format = Format::EVALUATION;
    CRTFTForwardTransformToBitReverseInPlace(ru, co, &(*m_values));
}

void CRTFFTForwardTransformToBitReverseInPlace(const IntType& rootOfUnity,const usint CycloOrder,VecType* element) {
    // std::cout << "ForwardTransformToBitReverseInPlace" << std::endl;

    if (rootOfUnity == IntType(1) || rootOfUnity == IntType(0)) {
        return;
    }

    if (!lbcrypto::IsPowerOfTwo(CycloOrder)) {
        OPENFHE_THROW(lbcrypto::math_error, "CyclotomicOrder is not a power of two");
    }

    usint CycloOrderHf = (CycloOrder >> 1);
    if (element->GetLength() != CycloOrderHf) {
        OPENFHE_THROW(lbcrypto::math_error, "element size must be equal to CyclotomicOrder / 2");
    }

    IntType modulus = element->GetModulus();

    auto mapSearch = m_rootOfUnityReverseTableByModulus.find(modulus);
    if (mapSearch == m_rootOfUnityReverseTableByModulus.end() || mapSearch->second.GetLength() != CycloOrderHf) {
        PreCompute(rootOfUnity, CycloOrder, modulus);
    }

    NTTForwardTransformToBitReverseInPlace(
        m_rootOfUnityReverseTableByModulus[modulus], m_rootOfUnityPreconReverseTableByModulus[modulus], element);
}



void NTTForwardTransformToBitReverseInPlace(NativeInt* rootOfUnityTable, NativeInt* preconRootOfUnityTable,
                                            NativeInt* element, NativeInt modulus, uint32_t length) {
    uint32_t n = length >> 1, t = n, logt = GetMSB(t);
    for (uint32_t m = 1; m < n; m <<= 1, t >>= 1, --logt) {
        for (uint32_t i = 0; i < m; ++i) {
            NativeInt omega       = rootOfUnityTable[i + m];
            NativeInt preconOmega = preconRootOfUnityTable[i + m];
            for (uint32_t j1 = i << logt, j2 = j1 + t; j1 < j2; ++j1) {
                NativeInt omegaFactor = element[j1 + t];
                ModMulFastConstEq(&omegaFactor, omega, modulus, preconOmega);
                NativeInt loVal = element[j1 + 0];
#if defined(__GNUC__) && !defined(__clang__)
                NativeInt hiVal = loVal + omegaFactor;
                if (hiVal >= modulus)
                    hiVal -= modulus;
                if (loVal < omegaFactor)
                    loVal += modulus;
                loVal -= omegaFactor;
                element[j1 + 0] = hiVal;
                element[j1 + t] = loVal;
#else
                // fixes Clang slowdown issue, but requires lowVal be less than modulus
                element[j1 + 0] += omegaFactor - (omegaFactor >= (modulus - loVal) ? modulus : 0);
                if (omegaFactor > loVal)
                    loVal += modulus;
                element[j1 + t] = loVal - omegaFactor;
#endif
            }
        }
    }
    for (uint32_t i = 0; i < (n << 1); i += 2) {
        NativeInt omegaFactor = element[i + 1];
        NativeInt omega       = rootOfUnityTable[(i >> 1) + n];
        NativeInt preconOmega = preconRootOfUnityTable[(i >> 1) + n];
        ModMulFastConstEq(&omegaFactor, omega, modulus, preconOmega);
        NativeInt loVal = element[i + 0];
#if defined(__GNUC__) && !defined(__clang__)
        NativeInt hiVal = loVal + omegaFactor;
        if (hiVal >= modulus)
            hiVal -= modulus;
        if (loVal < omegaFactor)
            loVal += modulus;
        loVal -= omegaFactor;
        element[i + 0] = hiVal;
        element[i + 1] = loVal;
#else
        element[i + 0] += omegaFactor - (omegaFactor >= (modulus - loVal) ? modulus : 0);
        if (omegaFactor > loVal)
            loVal += modulus;
        element[i + t] = loVal - omegaFactor;
#endif
    }
}

void NTTInverseTransformFromBitReverseInPlace(NativeInt* rootOfUnityInverseTable, NativeInt cycloOrderInv,
                                              NativeInt* element, NativeInt modulus, uint32_t length) {
    uint32_t n   = length;
    NativeInt mu = ComputeMu(modulus);

    NativeInt loVal, hiVal, omega, omegaFactor;
    uint8_t i, m, j1, j2, indexOmega, indexLo, indexHi;

    uint8_t t     = 1;
    uint8_t logt1 = 1;
    for (m = (n >> 1); m >= 1; m >>= 1) {
        for (i = 0; i < m; ++i) {
            j1         = i << logt1;
            j2         = j1 + t;
            indexOmega = m + i;
            omega      = rootOfUnityInverseTable[indexOmega];

            for (indexLo = j1; indexLo < j2; ++indexLo) {
                indexHi = indexLo + t;

                hiVal = element[indexHi];
                loVal = element[indexLo];

                omegaFactor = loVal;
                if (omegaFactor < hiVal) {
                    omegaFactor += modulus;
                }

                omegaFactor -= hiVal;

                loVal += hiVal;
                if (loVal >= modulus) {
                    loVal -= modulus;
                }

                ModMulFastEq(&omegaFactor, omega, modulus, mu);

                element[indexLo] = loVal;
                element[indexHi] = omegaFactor;
            }
        }
        t <<= 1;
        logt1++;
    }

    for (i = 0; i < n; i++) {
        ModMulFastEq(element + i, cycloOrderInv, modulus, mu);
    }
    return;
}