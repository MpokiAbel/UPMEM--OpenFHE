#include <dpu>
#include <iostream>

using namespace dpu;

// template <typename Element>
// Element run_on_dpu(Element& m_value_1, const Element& m_value_2) {
//     try {
//         auto system = DpuSet::allocate(1, "backend=simulator");
//         auto dpu    = system.dpus()[0];

//         std::vector<Element> data{m_value_1, m_value_2};
//         std::vector<std::vector<Element>> results{std::vector<Element>(1)};

//         dpu->load("./src/dpu/helloworld_dpu");
//         dpu->copy("my_var", data);
//         dpu->exec();
//         dpu->log(std::cout);
//         dpu->copy(results, "my_var");

//         return results[0][0];
//     }
//     catch (const DpuError& e) {
//         std::cerr << e.what() << std::endl;
//     }

//     return 0;
// }

int run_on_dpu() {
    try {
        auto dpu = DpuSet::allocate(1);
        dpu.load("./src/dpu/helloworld_dpu");
        dpu.exec();
        dpu.log(std::cout);
    }
    catch (const DpuError& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

template <typename Element>
std::vector<Element> run_on_dpu(std::vector<Element>& m_value_1, std::vector<Element>& m_value_2, Element modulus) {
    std::vector<Element> ret;
    try {
        auto system = DpuSet::allocate(1, "backend=simulator");
        auto dpu    = system.dpus()[0];

        std::vector<Element> m_modulus = {modulus};
        std::vector<std::vector<Element>> results{std::vector<Element>(m_value_1.size())};

        dpu->load("./src/dpu/mubintvecnat_dpu");
        std::cout << "LOADED THE DPU PROGRAM" << std::endl;
        dpu->copy("mram_array_1", m_value_1);
        dpu->copy("mram_array_2", m_value_2);
        dpu->copy("mram_modulus", m_modulus);
        dpu->exec();
        dpu->log(std::cout);
        dpu->copy(results, "mram_array_1");

        ret = results[0];
    }
    catch (const DpuError& e) {
        std::cerr << e.what() << std::endl;
    }

    return ret;
}
