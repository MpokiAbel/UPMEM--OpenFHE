
// template <typename Element>
// void Copy_Data_From_Dpus(intnat::NativeVectorT<Element>* a) {
//     size_t size                  = a->GetLength() / GetNumDpus();  // Calculate data size per DPU
//     size_t size_to_copy_in_bytes = size * sizeof(uint64_t);        // size of data in bytes to copy

//     // Retrieve data to be processed
//     TDvectorInt_64 results(GetNumDpus(), DvectorInt_64(size));  // Container for DPU data results

//     // Copy results from DPUs
//     system.copy(results, size_to_copy_in_bytes, DPU_MRAM_HEAP_POINTER_NAME);

//     size_t index = 0;
//     for (const auto& innerVec : results) {
//         for (auto elem : innerVec) {
//             // LOGv("Before ", (*a).at(index));
//             (*a).at(index) = elem;
//             // LOGv((*a).at(index), elem);
//             index++;
//             if (index > a->GetLength())
//                 break;
//         }
//         if (index > a->GetLength())
//             break;
//     }
// }

// template <typename Element>
// void Copy_Data_To_Dpus(intnat::NativeVectorT<Element>* a, intnat::NativeVectorT<Element> b, unsigned dpuSplit) {
//     auto size = a->GetLength() / GetNumDpus();  // Calculate data size per DPU, this number always has to divide
//     auto size_to_copy_in_bytes = size * sizeof(uint64_t);  // size of data in bytes to copy

//     TDvectorInt_64 a_data(GetNumDpus());
//     TDvectorInt_64 b_data(GetNumDpus());
//     DvectorInt_64 modulus{a->GetModulus().ConvertToInt()};
//     DvectorInt_64 data_to_copy{size_to_copy_in_bytes};  //  The values in bytes of the data copied

//     for (size_t i = 0; i < GetNumDpus(); i++) {
//         for (size_t j = 0; j < size; j++) {
//             a_data[i].push_back((*a).at((i * size) + j).ConvertToInt());
//             b_data[i].push_back(b.at((i * size) + j).ConvertToInt());
//         }
//     }

//     // Copy data to DPUs
//     system.copy("mram_modulus", modulus);
//     system.copy("data_copied_in_bytes", data_to_copy);
//     system.copy(DPU_MRAM_HEAP_POINTER_NAME, 0, a_data, size_to_copy_in_bytes);
//     system.copy(DPU_MRAM_HEAP_POINTER_NAME, size_to_copy_in_bytes, b_data, size_to_copy_in_bytes);

//     // LOGv("Data copied ", a_data[0].size());
// }
// // Explicit instantiation of the template for the specific type you are using.
// template int PimManager::Run_On_Pim(intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>* a,
//                                     const intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>& b);

// template void PimManager::Copy_Data_To_Dpus(intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>* a,
//                                             const intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>& b,
//                                             unsigned split);

// template void PimManager::Copy_Data_From_Dpus(intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>* a);