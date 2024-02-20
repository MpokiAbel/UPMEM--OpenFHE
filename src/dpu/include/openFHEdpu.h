#include <dpu>
#include <iostream>
using namespace dpu;

void run_on_dpu() {
    try {
        auto system = DpuSet::allocate(1, "backend=simulator");
        auto dpu    = system.dpus()[0];
        dpu->load("./src/dpu/helloworld_dpu");
        dpu->exec();
        dpu->log(std::cout);
    }
    catch (const DpuError& e) {
        std::cerr << e.what() << std::endl;
    }
}
