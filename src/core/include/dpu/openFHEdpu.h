#ifdef RUN_ON_DPU
    #include <dpu>
    #include <iostream>
    #include <chrono>
    #include <tuple>

    #ifndef DPU_BINARY
        #define DPU_BINARY "./src/core/dpu/mubintvecnat_dpu"
    #endif

using namespace dpu;

/* get_data() This function is made to extract the data from the type of data representations available i.e 
    1. DCRTPoly
    2. NativeVectorT
    3. Interger etc

    The current implementation assume we are dealing with NativeVectorT type more representations will be added
    for running on multiple layers of openFHE
*/

template <typename Element>
std::tuple<std::vector<uint64_t>, std::vector<uint64_t>, std::vector<uint64_t>> get_data(Element& a, const Element& b) {
    std::vector<uint64_t> a_data;
    std::vector<uint64_t> b_data;
    std::vector<uint64_t> modulus;

    modulus.push_back(a.GetModulus().toNativeInt());

    for (size_t i = 0; i < a.GetLength(); i++) {
        a_data.push_back(a[i].toNativeInt());
        b_data.push_back(b[i].toNativeInt());
    }

    return std::make_tuple(a_data, b_data, modulus);
}

/*
    Here we assume the usage of one DPU hence results is only stored on the first element of the outer vector
        Note: Before using multiple DPUs change this
*/

template <class Element, typename IntType>
void set_data(Element* a, std::vector<std::vector<IntType>> results) {
    for (size_t i = 0; i < a->GetLength(); i++) {
        (*a)[i].SetValue(std::to_string(results[0][i]));
    }
}

template <typename Element>
int run_on_pim(Element* a, const Element& b) {
    int ret = 0;

    try {
        auto system = DpuSet::allocate(1);
        // auto dpu    = system.dpus()[0];
        auto data = get_data(*a, b);

        std::vector<std::vector<uint64_t>> results{std::vector<uint64_t>(a->GetLength())};
        system.load(DPU_BINARY);
        system.copy("mram_array_1", std::get<0>(data));
        system.copy("mram_array_2", std::get<1>(data));
        system.copy("mram_modulus", std::get<2>(data));
        system.exec();
        // dpu->log(std::cout);
        system.copy(results, "mram_array_1");

        set_data<Element, uint64_t>(a, results);

        ret = 1;
    }
    catch (const DpuError& e) {
        std::cerr << e.what() << std::endl;
    }

    return ret;
}

#endif