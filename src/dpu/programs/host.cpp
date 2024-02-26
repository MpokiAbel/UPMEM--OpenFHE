#include <stddef.h>
#include <stdint.h>
#include <cstdint>
#include <vector>
#include <iostream>
#include <dpu>

using namespace dpu;

int main() {
    std::vector<uint64_t> value_1;
    std::vector<uint64_t> value_2;

    for (size_t i = 0; i < 10; i++) {
        value_1.push_back(i);
        value_2.push_back(i * 2);
    }

    std::vector<uint64_t> modulus = {10};

    try {
        auto system = DpuSet::allocate(1, "backend=simulator");
        auto dpu    = system.dpus()[0];

        std::vector<std::vector<uint64_t>> results{std::vector<uint64_t>(value_1.size())};

        dpu->load("helloworld_dpu");
        dpu->copy("mram_array_1", value_1);
        dpu->copy("mram_array_2", value_2);
        dpu->copy("mram_modulus", modulus);
        dpu->exec();
        dpu->log(std::cout);
        dpu->copy(results, "mram_array_1");

        value_1 = results[0];
    }
    catch (const DpuError& e) {
        std::cerr << e.what() << std::endl;
    }

    for (size_t i = 0; i < value_1.size(); i++) {
        std::cout << value_1[i] << std::endl;
    }

    return 0;
}
