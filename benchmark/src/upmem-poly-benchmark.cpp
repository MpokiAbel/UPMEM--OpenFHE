/*
 * This code benchmarks polynomial operations against UPMEM operations.
 */

#include <memory>
#define _USE_MATH_DEFINES
#include "vechelper.h"
#include "lattice/lat-hal.h"

#include <iostream>
#include <vector>

#include "benchmark/benchmark.h"

#define DPU_BINARY_DCRTadd "/home/pim/MPOKI/UPMEM-OpenFHE/build/src/core/Pim/dpu/mubintvecnatadd_dpu"
#define DPU_BINARY_DCRTmul "/home/pim/MPOKI/UPMEM-OpenFHE/build/src/core/Pim/dpu/mubintvecnatmul_dpu"

using namespace lbcrypto;

// static std::vector<usint> tow_args({1, 2, 4, 8, 16, 32});
static std::vector<usint> tow_args({1});
static std::vector<usint> dpu_args({1, 2, 4, 8, 16, 32, 64, 128, 256});
static const usint dpuNo        = 256;
static const usint iter         = 100;
static const usint DCRTBITS     = MAX_MODULUS_SIZE;
static const usint RING_DIM_LOG = 18;
static const size_t POLY_NUM    = 2;
static const size_t POLY_NUM_M1 = (POLY_NUM - 1);

static DCRTPoly makeElement(std::shared_ptr<ILDCRTParams<BigInteger>> p, Format format) {
    auto params   = std::make_shared<ILParams>(p->GetCyclotomicOrder(), p->GetModulus(), 1);
    BigVector vec = makeVector<BigVector>(params->GetRingDimension(), params->GetModulus());

    DCRTPoly::PolyLargeType bigE(params);
    bigE.SetValues(vec, format);

    DCRTPoly elem(bigE, p);
    return elem;
}

static void GenerateDCRTParms(std::map<usint, std::shared_ptr<ILDCRTParams<BigInteger>>>& parmArray) {
    for (usint t : tow_args) {
        uint32_t m = (1 << (RING_DIM_LOG + 1));

        std::vector<NativeInteger> moduli(t);
        std::vector<NativeInteger> roots(t);

        NativeInteger firstInteger = FirstPrime<NativeInteger>(DCRTBITS, m);
        moduli[0]                  = PreviousPrime<NativeInteger>(firstInteger, m);
        roots[0]                   = RootOfUnity<NativeInteger>(m, moduli[0]);

        for (size_t i = 1; i < t; i++) {
            moduli[i] = PreviousPrime<NativeInteger>(moduli[i - 1], m);
            roots[i]  = RootOfUnity<NativeInteger>(m, moduli[i]);
        }

        ChineseRemainderTransformFTT<NativeVector>().PreCompute(roots, m, moduli);

        parmArray[t] = std::make_shared<ILDCRTParams<BigInteger>>(m, moduli, roots);
    }
}

static void GenerateDCRTPolys(std::map<usint, std::shared_ptr<ILDCRTParams<BigInteger>>>& parmArray,
                              std::map<usint, std::shared_ptr<std::vector<DCRTPoly>>>& polyArrayEval,
                              std::map<usint, std::shared_ptr<std::vector<DCRTPoly>>>& polyArrayCoef) {
    for (auto& pair : parmArray) {
        std::vector<DCRTPoly> vecEval;
        for (size_t i = 0; i < POLY_NUM; i++) {
            vecEval.push_back(makeElement(parmArray[pair.first], Format::EVALUATION));
        }
        polyArrayEval[pair.first] = std::make_shared<std::vector<DCRTPoly>>(std::move(vecEval));
        std::vector<DCRTPoly> vecCoef;
        for (size_t i = 0; i < POLY_NUM; i++) {
            vecCoef.push_back(makeElement(parmArray[pair.first], Format::COEFFICIENT));
        }
        polyArrayCoef[pair.first] = std::make_shared<std::vector<DCRTPoly>>(std::move(vecCoef));
    }
}

std::map<usint, std::shared_ptr<ILDCRTParams<BigInteger>>> DCRTparms;

std::map<usint, std::shared_ptr<std::vector<DCRTPoly>>> DCRTpolysEval;

std::map<usint, std::shared_ptr<std::vector<DCRTPoly>>> DCRTpolysCoef;

std::shared_ptr<PimManager> pim;

class Setup {
public:
    Setup() {
        GenerateDCRTParms(DCRTparms);
        std::cerr << "Generating polynomials for the benchmark..." << std::endl;
        GenerateDCRTPolys(DCRTparms, DCRTpolysEval, DCRTpolysCoef);
        pim = std::make_unique<PimManager>(dpuNo);
        std::cerr << "Polynomials for the benchmark are generated" << std::endl;
    }
} TestParameters;

static void DCRTArguments(benchmark::internal::Benchmark* b) {
    for (usint t : tow_args) {
        b->ArgName("towers")->Arg(t);
    }
}

static void DCRTPIMArguments(benchmark::internal::Benchmark* b) {
    for (usint t : tow_args) {
        for (usint d : dpu_args) {
            if ((t * d) != dpuNo)
                continue;
            b->ArgName("towers")->Args({t, d});
        }
    }
}

static void DCRT_add(benchmark::State& state) {  // benchmark
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];

    /*Just To keep in mind the number of towers is the number of m_vectors of a Specific DCRTPoly*/

    DCRTPoly *a, *b;
    size_t i = 0;
    a        = &(polys->operator[](i));
    b        = &(polys->operator[](i + 1));
    while (state.KeepRunning()) {
        (*a) += (*b);
    }
}

BENCHMARK(DCRT_add)->Unit(benchmark::kMillisecond)->Apply(DCRTArguments);

static void DCRT_add_PIM_WO_COPY(benchmark::State& state) {  // benchmark
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];

    /*Just To keep in mind the number of towers is the number of m_vectors of a Specific DCRTPoly*/
    pim->Load_Binary_To_Dpus(DPU_BINARY_DCRTadd);

    DCRTPoly *a, *b;
    size_t i = 0;
    a        = &(polys->operator[](i));
    b        = &(polys->operator[](i + 1));

    size_t dpuNum            = state.range(1) * a->GetAllElements().size();
    size_t towerElementCount = a->GetAllElements()[0].GetValues().GetLength();
    size_t towerSplitCount   = towerElementCount / dpuNum;

    vector2D result(pim->GetNumDpus(), vector1D(towerSplitCount));
    DPUData dataDPU;

    pim->Prepare_Data_For_Dpus(*a, *b, dataDPU, state.range(1));
    pim->Copy_Data_To_Dpus(dataDPU);

    for (auto _ : state) {
        pim->Execute_On_Dpus();
    }

    pim->Copy_From_Dpus(towerSplitCount, result);
    pim->fill_DCRTPoly(*a, result, state.range(1));
}

BENCHMARK(DCRT_add_PIM_WO_COPY)->Unit(benchmark::kMillisecond)->Apply(DCRTPIMArguments);

static void DCRT_add_PIM_W_COPY(benchmark::State& state) {  // benchmark
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];

    /*Just To keep in mind the number of towers is the number of m_vectors of a Specific DCRTPoly*/
    pim->Load_Binary_To_Dpus(DPU_BINARY_DCRTadd);
    DCRTPoly *a, *b;
    size_t i = 0;
    a        = &(polys->operator[](i));
    b        = &(polys->operator[](i + 1));

    size_t dpuNum            = state.range(1) * a->GetAllElements().size();
    size_t towerElementCount = a->GetAllElements()[0].GetValues().GetLength();
    size_t towerSplitCount   = towerElementCount / dpuNum;

    vector2D result(pim->GetNumDpus(), vector1D(towerSplitCount));
    DPUData dataDPU;
    pim->Prepare_Data_For_Dpus(*a, *b, dataDPU, state.range(1));

    for (auto _ : state) {
        pim->Copy_Data_To_Dpus(dataDPU);
        pim->Execute_On_Dpus();
        pim->Copy_From_Dpus(towerSplitCount, result);
    }
    pim->fill_DCRTPoly(*a, result, state.range(1));
}
BENCHMARK(DCRT_add_PIM_W_COPY)->Unit(benchmark::kMillisecond)->Apply(DCRTPIMArguments);

static void DCRT_add_PIM_Copy_to_DPUs(benchmark::State& state) {  // benchmark
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];
    pim->Load_Binary_To_Dpus(DPU_BINARY_DCRTadd);

    /*Just To keep in mind the number of towers is the number of m_vectors of a Specific DCRTPoly*/
    DCRTPoly *a, *b;
    a = &(polys->operator[](0));
    b = &(polys->operator[](1));

    size_t dpuNum            = state.range(1) * a->GetAllElements().size();
    size_t towerElementCount = a->GetAllElements()[0].GetValues().GetLength();
    size_t towerSplitCount   = towerElementCount / dpuNum;

    vector2D result(pim->GetNumDpus(), vector1D(towerSplitCount));
    DPUData dpuData;
    pim->Prepare_Data_For_Dpus(*a, *b, dpuData, state.range(1));

    for (auto _ : state) {
        pim->Copy_Data_To_Dpus(dpuData);
    }
    pim->Execute_On_Dpus();
    pim->Copy_From_Dpus(towerSplitCount, result);
    pim->fill_DCRTPoly(*a, result, state.range(1));
}

BENCHMARK(DCRT_add_PIM_Copy_to_DPUs)->Unit(benchmark::kMillisecond)->Apply(DCRTPIMArguments);

static void DCRT_add_PIM_Copy_from_DPUs(benchmark::State& state) {  // benchmark
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];
    pim->Load_Binary_To_Dpus(DPU_BINARY_DCRTadd);

    /*Just To keep in mind the number of towers is the number of m_vectors of a Specific DCRTPoly*/
    DCRTPoly *a, *b;
    size_t i = 0;
    a        = &(polys->operator[](i));
    b        = &(polys->operator[](i + 1));

    size_t dpuNum            = state.range(1) * a->GetAllElements().size();
    size_t towerElementCount = a->GetAllElements()[0].GetValues().GetLength();
    size_t towerSplitCount   = towerElementCount / dpuNum;

    vector2D result(pim->GetNumDpus(), vector1D(towerSplitCount));
    DPUData dpuData;
    pim->Prepare_Data_For_Dpus(*a, *b, dpuData, state.range(1));
    pim->Copy_Data_To_Dpus(dpuData);

    pim->Execute_On_Dpus();
    pim->Copy_From_Dpus(towerSplitCount, result);

    for (auto _ : state) {
        pim->fill_DCRTPoly(*a, result, state.range(1));
    }
}
BENCHMARK(DCRT_add_PIM_Copy_from_DPUs)->Unit(benchmark::kMillisecond)->Apply(DCRTPIMArguments);

// static void DPU_ALLOCATION(benchmark::State& state) {
//     for (auto _ : state) {
//         pim->Load_Binary_To_Dpus(DPU_BINARY_DCRTadd);
//     }
// }

static void DCRT_mul(benchmark::State& state) {
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];
    pim->Load_Binary_To_Dpus(DPU_BINARY_DCRTmul);

    DCRTPoly *a, *b;
    size_t i = 0;
    a        = &(polys->operator[](i));
    b        = &(polys->operator[](i + 1));
    while (state.KeepRunning()) {
        (*a) *= (*b);
    }
}
BENCHMARK(DCRT_mul)->Unit(benchmark::kMillisecond)->Apply(DCRTArguments);

static void DCRT_mul_PIM_WO_COPY(benchmark::State& state) {
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];
    pim->Load_Binary_To_Dpus(DPU_BINARY_DCRTmul);

    DCRTPoly *a, *b;
    size_t i = 0;
    a        = &(polys->operator[](i));
    b        = &(polys->operator[](i + 1));

    size_t dpuNum            = state.range(1) * a->GetAllElements().size();
    size_t towerElementCount = a->GetAllElements()[0].GetValues().GetLength();
    size_t towerSplitCount   = towerElementCount / dpuNum;

    vector2D result(pim->GetNumDpus(), vector1D(towerSplitCount));
    DPUData dpuData;
    pim->Prepare_Data_For_Dpus(*a, *b, dpuData, state.range(1));
    pim->Copy_Data_To_Dpus(dpuData);

    while (state.KeepRunning()) {
        pim->Execute_On_Dpus();
    }

    pim->Copy_From_Dpus(towerSplitCount, result);
    pim->fill_DCRTPoly(*a, result, state.range(1));
}
BENCHMARK(DCRT_mul_PIM_WO_COPY)->Unit(benchmark::kMillisecond)->Apply(DCRTPIMArguments);

static void DCRT_mul_PIM_W_COPY(benchmark::State& state) {
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];
    pim->Load_Binary_To_Dpus(DPU_BINARY_DCRTmul);

    DCRTPoly *a, *b;
    size_t i = 0;
    a        = &(polys->operator[](i));
    b        = &(polys->operator[](i + 1));

    size_t dpuNum            = state.range(1) * a->GetAllElements().size();
    size_t towerElementCount = a->GetAllElements()[0].GetValues().GetLength();
    size_t towerSplitCount   = towerElementCount / dpuNum;

    vector2D result(pim->GetNumDpus(), vector1D(towerSplitCount));
    DPUData dpuData;
    pim->Prepare_Data_For_Dpus(*a, *b, dpuData, state.range(1));

    while (state.KeepRunning()) {
        pim->Copy_Data_To_Dpus(dpuData);
        pim->Execute_On_Dpus();
        pim->Copy_From_Dpus(towerSplitCount, result);
    }

    pim->fill_DCRTPoly(*a, result, state.range(1));
}
BENCHMARK(DCRT_mul_PIM_W_COPY)->Unit(benchmark::kMillisecond)->Apply(DCRTPIMArguments);

// static void DCRT_ntt(benchmark::State& state) {
//     std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysCoef[state.range(0)];
//     DCRTPoly a;
//     size_t i = 0;
//     a        = polys->operator[](i);
//     // i++;
//     // i = i & POLY_NUM_M1;

//     while (state.KeepRunning()) {
//         a.SwitchFormat();
//     }
// }
// BENCHMARK(DCRT_ntt)->Unit(benchmark::kMillisecond)->Apply(DCRTArguments);

// static void DCRT_ntt_PIM(benchmark::State& state) {
//     std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysCoef[state.range(0)];
//     DCRTPoly a;
//     size_t i = 0;

//     while (state.KeepRunning()) {
//         a = polys->operator[](i);
//         i++;
//         i = i & POLY_NUM_M1;
//         a.SwitchFormat();
//     }
// }

// static void DCRT_intt(benchmark::State& state) {
//     std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];
//     DCRTPoly a;
//     size_t i = 0;

//     while (state.KeepRunning()) {
//         a = polys->operator[](i);
//         i++;
//         i = i & POLY_NUM_M1;
//         a.SwitchFormat();
//     }
// }

// static void DCRT_intt_PIM(benchmark::State& state) {
//     std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];
//     DCRTPoly a;
//     size_t i = 0;

//     while (state.KeepRunning()) {
//         a = polys->operator[](i);
//         i++;
//         i = i & POLY_NUM_M1;
//         a.SwitchFormat();
//     }
// }

BENCHMARK_MAIN();