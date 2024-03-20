#include <cstddef>
#include <cstdint>
#define PROFILE
#define _USE_MATH_DEFINES
#include "scheme/bfvrns/cryptocontext-bfvrns.h"
#include "scheme/bgvrns/cryptocontext-bgvrns.h"
#include "gen-cryptocontext.h"

#include "benchmark/benchmark.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <unistd.h>  // for linux

using namespace lbcrypto;

/*
 * Context setup utility methods
 */

CryptoContext<DCRTPoly> GenerateBFVrnsContext() {
    CCParams<CryptoContextBFVRNS> parameters;
    parameters.SetPlaintextModulus(65537);
    parameters.SetMultiplicativeDepth(4);
    // parameters.SetScalingModSize(60);
    parameters.SetSecurityLevel(HEStd_256_classic);

    CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);
    // Enable features that you wish to use
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);

    return cc;
}

/*
    Evaluation of BFVrns_Add
*/

void BFVrns_Add_CPU(benchmark::State& state) {
    CryptoContext<DCRTPoly> cc = GenerateBFVrnsContext();

    KeyPair<DCRTPoly> keyPair = cc->KeyGen();

    std::vector<int64_t> vectorOfInts1 = {1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0};
    std::vector<int64_t> vectorOfInts2 = {0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};

    auto plaintext1 = cc->MakePackedPlaintext(vectorOfInts1);
    auto plaintext2 = cc->MakePackedPlaintext(vectorOfInts2);

    auto ciphertext1 = cc->Encrypt(keyPair.publicKey, plaintext1);
    auto ciphertext2 = cc->Encrypt(keyPair.publicKey, plaintext2);

    while (state.KeepRunning()) {
        auto ciphertextAdd = cc->EvalAdd(ciphertext1, ciphertext2);
        benchmark::DoNotOptimize(ciphertextAdd);
    }
}

BENCHMARK(BFVrns_Add_CPU)->Unit(benchmark::kMillisecond);

void BFVrns_Add_Pim(benchmark::State& state) {
    CryptoContext<DCRTPoly> cc = GenerateBFVrnsContext();

    KeyPair<DCRTPoly> keyPair = cc->KeyGen();

    std::vector<int64_t> vectorOfInts1 = {1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0};
    std::vector<int64_t> vectorOfInts2 = {0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};

    auto plaintext1 = cc->MakePackedPlaintext(vectorOfInts1);
    auto plaintext2 = cc->MakePackedPlaintext(vectorOfInts2);

    auto ciphertext1 = cc->Encrypt(keyPair.publicKey, plaintext1);
    auto ciphertext2 = cc->Encrypt(keyPair.publicKey, plaintext2);

    // Calculate data size to be offloaded to DPUs
    size_t sizev = ciphertext1->GetElements()[0].GetAllElements().size();

    size_t dataSize =
        sizev * ciphertext1->GetElements()[0].GetAllElements()[0].GetValues().GetLength() * sizeof(std::uint64_t);

    std::cout << "Data Size is " << dataSize / (1024 * 1024) << "MB" << std::endl;

    std::shared_ptr<PimManager> pim = std::make_shared<PimManager>(sizev * 32);

    ciphertext1->SetPim(pim);

    while (state.KeepRunning()) {
        auto ciphertextAdd = cc->EvalAdd(ciphertext1, ciphertext2);
        benchmark::DoNotOptimize(ciphertextAdd);
    }
}

BENCHMARK(BFVrns_Add_Pim)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();