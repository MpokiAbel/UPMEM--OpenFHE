#include <dpu>
#include <iostream>

using namespace dpu;

template <typename Element>
Element run_on_dpu(Element& m_value_1, const Element& m_value_2) {
    try {
        auto system = DpuSet::allocate(1, "backend=simulator");
        auto dpu    = system.dpus()[0];

        std::vector<Element> data{m_value_1, m_value_2};
        std::vector<std::vector<Element>> results{std::vector<Element>(1)};

        dpu->load("./src/dpu/helloworld_dpu");
        dpu->copy("my_var", data);
        dpu->exec();
        dpu->log(std::cout);
        dpu->copy(results, "my_var");

        return results[0][0];
    }
    catch (const DpuError& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
