//==================================================================================
// BSD 2-Clause License
//
// Copyright (c) 2014-2022, NJIT, Duality Technologies Inc. and other contributors
//
// All rights reserved.
//
// Author TPOC: contact@openfhe.org
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//==================================================================================

/*
  Simple example for BFVrns (integer arithmetic)
 */
#include "openfhe.h"
#include <cstddef>
#include <iostream>
#include <chrono>

using namespace lbcrypto;

template <typename T>
double VectorSizeInMB(const std::vector<T>& v) {
    // Size of the vector in bytes = number of elements * size of each element
    size_t totalSizeInBytes = v.size() * sizeof(T);

    // Convert bytes to megabytes
    double totalSizeInMB = static_cast<double>(totalSizeInBytes) / (1024 * 1024);

    return totalSizeInMB;
}

int main() {
    // Sample Program: Step 1: Set CryptoContext
    CCParams<CryptoContextBFVRNS> parameters;
    parameters.SetPlaintextModulus(65537);
    parameters.SetMultiplicativeDepth(2);

    CryptoContext<DCRTPoly> cryptoContext = GenCryptoContext(parameters);
    // Enable features that you wish to use
    cryptoContext->Enable(PKE);
    cryptoContext->Enable(KEYSWITCH);
    cryptoContext->Enable(LEVELEDSHE);

    // Sample Program: Step 2: Key Generation

    // Initialize Public Key Containers
    KeyPair<DCRTPoly> keyPair;

    // Generate a public/private key pair
    keyPair = cryptoContext->KeyGen();

    // Generate the relinearization key
    cryptoContext->EvalMultKeyGen(keyPair.secretKey);

    // Generate the rotation evaluation keys
    cryptoContext->EvalRotateKeyGen(keyPair.secretKey, {1, 2, -1, -2});

    // Sample Program: Step 3: Encryption

    size_t size = 12;
    // First plaintext vector is encoded
    std::vector<int64_t> vectorOfInts1;
    vectorOfInts1.reserve(size);

    // Second plaintext vector is encoded
    std::vector<int64_t> vectorOfInts2;
    vectorOfInts2.reserve(size);

    for (size_t i = 0; i < size; i++) {
        vectorOfInts1.push_back(2);
        vectorOfInts2.push_back(1);
    }

    // std::cout << "Total size of the message in MB: " << VectorSizeInMB(vectorOfInts1) << std::endl;
    // auto start           = timeNow();
    Plaintext plaintext2 = cryptoContext->MakePackedPlaintext(vectorOfInts2);

    Plaintext plaintext1 = cryptoContext->MakePackedPlaintext(vectorOfInts1);
    // auto end             = timeNow();
    // std::cout << "Plaintext encoding takes " << duration_ms(end - start) << " ms.\n";
    // Third plaintext vector is encoded
    // std::vector<int64_t> vectorOfInts3 = {1, 2, 5, 2, 5, 6, 7, 8, 9, 10, 11, 12};
    // Plaintext plaintext3               = cryptoContext->MakePackedPlaintext(vectorOfInts3);

    // std::cout<<"I am about to do Encryption"<<std::endl;
    // The encoded vectors are encrypted

    auto ciphertext1 = cryptoContext->Encrypt(keyPair.publicKey, plaintext1);
    auto ciphertext2 = cryptoContext->Encrypt(keyPair.publicKey, plaintext2);

    // auto ciphertext3 = cryptoContext->Encrypt(keyPair.publicKey, plaintext3);
    // std::cout<<"Finished Encryption"<<std::endl;

    // Sample Program: Step 4: Evaluation

    // std::cout<<"I am about to do addition"<<std::endl;
    // Homomorphic additions

    // std::vector<std::shared_ptr<PimManager>> pim;
    // size_t sizev = ciphertext1->GetElements()[0].GetAllElements().size();  // Assuming this gets the intended size
    // for (size_t i = 0; i < sizev; ++i) {
    //     pim.push_back(std::make_shared<PimManager>(1));
    // }

    // ciphertext1->SetPim(pim);
    // std::cout << "Number of DPUS " << pim->GetNumDpus() << std::endl;
    // std::cout << "The number of DPUs or towers " << sizev << std::endl;
    // std::shared_ptr<PimManager> pim = std::make_shared<PimManager>(sizev);
    // ciphertext1->SetPim(pim);

    // auto start               = timeNow();
    auto ciphertextAddResult = cryptoContext->EvalAdd(ciphertext1, ciphertext2);
    // auto end                 = timeNow();
    // std::cout << "EvalAdd takes " << duration_ms(end - start) << " ms.\n";

    // auto ciphertextAddResult = cryptoContext->EvalAdd(ciphertextAdd12, ciphertext3);
    // std::cout<<"Finished addition"<<std::endl;

    // Homomorphic multiplications
    // auto ciphertextMul12      = cryptoContext->EvalMult(ciphertext1, ciphertext2);
    // auto ciphertextMultResult = cryptoContext->EvalMult(ciphertextMul12, ciphertext3);

    // Homomorphic rotations
    // auto ciphertextRot1 = cryptoContext->EvalRotate(ciphertext1, 1);
    // auto ciphertextRot2 = cryptoContext->EvalRotate(ciphertext1, 2);
    // auto ciphertextRot3 = cryptoContext->EvalRotate(ciphertext1, -1);
    // auto ciphertextRot4 = cryptoContext->EvalRotate(ciphertext1, -2);

    // Sample Program: Step 5: Decryption

    // Decrypt the result of additions
    Plaintext plaintextAddResult;
    cryptoContext->Decrypt(keyPair.secretKey, ciphertextAddResult, &plaintextAddResult);

    // // Decrypt the result of multiplications
    // Plaintext plaintextMultResult;
    // cryptoContext->Decrypt(keyPair.secretKey, ciphertextMultResult, &plaintextMultResult);

    // // Decrypt the result of rotations
    // Plaintext plaintextRot1;
    // cryptoContext->Decrypt(keyPair.secretKey, ciphertextRot1, &plaintextRot1);
    // Plaintext plaintextRot2;
    // cryptoContext->Decrypt(keyPair.secretKey, ciphertextRot2, &plaintextRot2);
    // Plaintext plaintextRot3;
    // cryptoContext->Decrypt(keyPair.secretKey, ciphertextRot3, &plaintextRot3);
    // Plaintext plaintextRot4;
    // cryptoContext->Decrypt(keyPair.secretKey, ciphertextRot4, &plaintextRot4);

    // plaintextRot1->SetLength(vectorOfInts1.size());
    // plaintextRot2->SetLength(vectorOfInts1.size());
    // plaintextRot3->SetLength(vectorOfInts1.size());
    // plaintextRot4->SetLength(vectorOfInts1.size());

    // std::cout << "Plaintext #1: " << plaintext1 << std::endl;
    // std::cout << "Plaintext #2: " << plaintext2 << std::endl;
    // std::cout << "Plaintext #3: " << plaintext3 << std::endl;

    // // Output results
    std::cout << "\nResults of homomorphic computations" << std::endl;
    std::cout << "#1 + #2 + #3: " << plaintextAddResult << std::endl;
    // std::cout << "#1 * #2 * #3: " << plaintextMultResult << std::endl;
    // std::cout << "Left rotation of #1 by 1: " << plaintextRot1 << std::endl;
    // std::cout << "Left rotation of #1 by 2: " << plaintextRot2 << std::endl;
    // std::cout << "Right rotation of #1 by 1: " << plaintextRot3 << std::endl;
    // std::cout << "Right rotation of #1 by 2: " << plaintextRot4 << std::endl;

    return 0;
}
