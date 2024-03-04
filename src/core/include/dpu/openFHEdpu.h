#ifdef RUN_ON_DPU
    #include <dpu>
    #include <iostream>
    #include <chrono>
    #include <tuple>
    #include <fstream>

    #ifndef DPU_BINARY
        #define DPU_BINARY "./src/core/dpu/mubintvecnat_dpu"
    #endif

    #define LOG(x) std::cout << x << std::endl
using namespace dpu;

/* get_data() This function is made to extract the data from the type of data representations available i.e 
    1. DCRTPoly
    2. Poly
    3. NativeVectorT
    3. Interger etc

    The current implementation assume we are dealing with NativeVectorT type more representations will be added
    for running on multiple layers of openFHE
*/

template <typename Element>
std::tuple<std::vector<std::vector<uint64_t>>, std::vector<std::vector<uint64_t>>, std::vector<uint64_t>> get_data(
    Element& a, const Element& b, size_t n_buffers) {
    size_t size = a.GetLength() / n_buffers;

    std::vector<std::vector<uint64_t>> a_data(n_buffers);
    std::vector<std::vector<uint64_t>> b_data(n_buffers);
    std::vector<uint64_t> modulus{a.GetModulus().toNativeInt()};

    for (size_t i = 0; i < n_buffers; i++) {
        a_data[i].reserve(size);
        b_data[i].reserve(size);
        for (size_t j = 0; j < size; j++) {
            a_data[i].push_back(a[(i * size) + j].toNativeInt());
            b_data[i].push_back(b[(i * size) + j].toNativeInt());
        }
    }
    return std::make_tuple(a_data, b_data, modulus);
}

/*
    Here we assume the usage of one DPU hence results is only stored on the first element of the outer vector
        Note: Before using multiple DPUs change this
*/

template <class Element, typename IntType>
void set_data(Element* a, std::vector<std::vector<IntType>> results) {
    size_t index = 0;
    for (const auto& innerVec : results) {
        for (auto elem : innerVec) {
            (*a)[index].SetValue(std::to_string(elem));
            index++;
            if (index >= a->GetLength())
                break;
        }
        if (index >= a->GetLength())
            break;
    }
}

template <typename Element>
int run_on_pim(Element* a, const Element& b) {
    int ret = 0;  // Variable to hold the return value of this function, indicating success or failure
    std::ofstream outFile("logs.txt", std::ios::app);  // Log file opened in append mode

    try {
        LOG("\nEntry to run dpu");
        // Allocation and initialization
        const int num_dpus  = 1;  // Assuming a single DPU for simplicity; adjust as needed
        auto system         = DpuSet::allocate(num_dpus, "backend=simulator");  // Allocate DPUs
        auto dpus_allocated = system.dpus().size();             // Number of successfully allocated DPUs
        auto data_size      = a->GetLength() / dpus_allocated;  // Calculate data size per DPU
        auto size_to_copy   = data_size * sizeof(uint64_t);     // size of data in bytes to copy

        // Data preparation
        auto data = get_data(*a, b, dpus_allocated);  // Retrieve data to be processed
        std::vector<std::vector<uint64_t>> results{dpus_allocated,
                                                   std::vector<uint64_t>(data_size)};  // Container for DPU results

        // Load binary to DPUs
        system.load(DPU_BINARY);

        // Copy data to DPUs
        system.copy("mram_modulus", std::get<2>(data));
        std::cout << "Allocated dpus: " << dpus_allocated << " Data Size: " << data_size << " size to copy "
                  << size_to_copy << std::endl;

        for (size_t index = 0; index < dpus_allocated; ++index) {
            system.dpus()[index]->copy("mram_array_1", std::get<0>(data)[index]);
            system.dpus()[index]->copy("mram_array_2", std::get<1>(data)[index]);
        }
        LOG("Done copying !!");

        // Execute the loaded program on all DPUs
        system.exec();
        system.log(outFile);

        // Copy results from DPUs
        system.copy(results, size_to_copy, "mram_array_1");
        LOG("Done fetching from DPU !! \n");

        // Post-processing
        set_data<Element, uint64_t>(a, results);

        ret = 1;
    }
    catch (const DpuError& e) {
        std::cerr << e.what() << std::endl;
    }

    return ret;
}
#endif