/*
 * This code benchmarks polynomial operations for ring dimension of 1k.
 */

#define _USE_MATH_DEFINES
#include "vechelper.h"
#include "lattice/lat-hal.h"

#include <iostream>
#include <vector>

#include "benchmark/benchmark.h"

using namespace lbcrypto;

static std::vector<usint> tow_args({1});

static const usint DCRTBITS     = MAX_MODULUS_SIZE;
static const usint RING_DIM_LOG = 20;
static const size_t POLY_NUM    = 16;
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
        pim = std::make_shared<PimManager>(512);
        std::cerr << "Polynomials for the benchmark are generated" << std::endl;
    }
} TestParameters;

static void DCRTArguments(benchmark::internal::Benchmark* b) {
    for (usint t : tow_args) {
        b->ArgName("towers")->Arg(t);
    }
}
static void DCRT_add(benchmark::State& state) {  // benchmark
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];

    /*Just To keep in mind the number of towers is the number of m_vectors of a Specific DCRTPoly*/

    DCRTPoly *a, *b;
    size_t i = 0;

    while (state.KeepRunning()) {
        a = &(polys->operator[](i));
        b = &(polys->operator[](i + 1));
        i += 2;
        i = i & POLY_NUM_M1;
        (*a) += (*b);
    }
}

static void DCRT_add_PIM(benchmark::State& state) {  // benchmark
    std::shared_ptr<std::vector<DCRTPoly>> polys = DCRTpolysEval[state.range(0)];

    /*Just To keep in mind the number of to256wers is the number of m_vectors of a Specific DCRTPoly*/
    DCRTPoly *a, *b;
    size_t i = 0;

    while (state.KeepRunning()) {
        a = &(polys->operator[](i));
        a->SetPim(pim);
        b = &(polys->operator[](i + 1));
        i += 2;
        i = i & POLY_NUM_M1;
        (*a) += (*b);
    }
}

BENCHMARK(DCRT_add)->Unit(benchmark::kMillisecond)->Apply(DCRTArguments);
BENCHMARK(DCRT_add_PIM)->Unit(benchmark::kMillisecond)->Apply(DCRTArguments);

BENCHMARK_MAIN();