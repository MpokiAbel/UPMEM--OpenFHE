#ifndef PIMMANAGER_H
#define PIMMANAGER_H

#define ALLOCATE_ALL_DPUS 0

#include <cstddef>
class PimManager {
public:
    PimManager();
    PimManager(uint32_t num_dpus, const std::string& profile = "");
    ~PimManager();

    void Load_Binary_To_Dpus(const std::string& binary);
    size_t GetNumDpus();

    template <typename Element>
    void Copy_Data_To_Dpus(Element* a, const Element& b, unsigned split = 1);

    void Execute_On_Dpus();

    template <typename Element>
    void Copy_Data_From_Dpus(Element* a,unsigned split=1);

    template <typename Element>
    int Run_On_Pim(Element* a, const Element& b);

private:
    class PimManagerImpl;    // Forward declaration of the implementation class
    PimManagerImpl* PmImpl;  // Pointer to the implementation
};

#endif  // PIMMANAGER_H