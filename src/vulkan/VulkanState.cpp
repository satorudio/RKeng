#include "VulkanState.h"

namespace RKeng
{
    VulkanState& GetVulkanState()
    {
        static VulkanState s_State;
        return s_State;
    }
}
