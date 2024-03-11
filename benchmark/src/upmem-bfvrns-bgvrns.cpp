

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

using namespace lbcrypto;

/*
 * Context setup utility methods
 */

CryptoContext<DCRTPoly> GenerateBFVrnsContext() {
    CCParams<CryptoContextBFVRNS> parameters;
    parameters.SetPlaintextModulus(65537);
    parameters.SetScalingModSize(60);

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

void BFVrns_Add(benchmark::State& state) {
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
    }
}
BENCHMARK(BFVrns_Add)->Unit(benchmark::kMicrosecond);


void BFVrns_Add_upmem(benchmark::State& state) {
    CryptoContext<DCRTPoly> cc = GenerateBFVrnsContext();

    KeyPair<DCRTPoly> keyPair = cc->KeyGen();

    std::vector<int64_t> vectorOfInts1 = {1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0};
    std::vector<int64_t> vectorOfInts2 = {0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};

    auto plaintext1 = cc->MakePackedPlaintext(vectorOfInts1);
    auto plaintext2 = cc->MakePackedPlaintext(vectorOfInts2);

    auto ciphertext1 = cc->Encrypt(keyPair.publicKey, plaintext1);
    auto ciphertext2 = cc->Encrypt(keyPair.publicKey, plaintext2);

    ciphertext1->SetOperation(1);
    ciphertext2->SetOperation(1);

    while (state.KeepRunning()) {
        auto ciphertextAdd = cc->EvalAdd(ciphertext1, ciphertext2);
    }
}

BENCHMARK(BFVrns_Add_upmem)->Unit(benchmark::kMicrosecond);


BENCHMARK_MAIN();