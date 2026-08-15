#include "VulkanGraphicsPipeline.h"

namespace cbk::platform::vk {

    void VulkanGraphicsPipeline::init(const VulkanDeviceContext& vkDeviceContext) {
        m_UBO.init(sizeof(SceneData), 0);
    }

    void VulkanGraphicsPipeline::shutdown() {
        
    }
}