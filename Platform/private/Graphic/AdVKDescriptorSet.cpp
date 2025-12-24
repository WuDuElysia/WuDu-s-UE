#include "Graphic/AdVKDescriptorSet.h"
#include "Graphic/AdVKDevice.h"

namespace WuDu {

        /**
         * @brief 构造函数，用于创建 Vulkan 描述符集布局对象
         * @param device 指向 Vulkan 设备对象的指针，用于获取逻辑设备句柄
         * @param bindings 描述符集布局绑定信息的数组，定义了每个绑定的描述符类型、数量和阶段等
         */
        AdVKDescriptorSetLayout::AdVKDescriptorSetLayout(AdVKDevice* device, const std::vector<VkDescriptorSetLayoutBinding>& bindings) : mDevice(device) {
                // 填充描述符集布局创建信息结构体
                VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .bindingCount = static_cast<uint32_t>(bindings.size()),
                        .pBindings = bindings.data()
                };
                // 调用 Vulkan API 创建描述符集布局，并检查结果
                CALL_VK(vkCreateDescriptorSetLayout(mDevice->GetHandle(), &descriptorSetLayoutInfo, nullptr, &mHandle));
        }

        /**
         * @brief 析构函数，用于销毁 Vulkan 描述符集布局对象
         */
        AdVKDescriptorSetLayout::~AdVKDescriptorSetLayout() {
                // 使用宏 VK_D 销毁描述符集布局对象
                VK_D(DescriptorSetLayout, mDevice->GetHandle(), mHandle);
        }

        /**
         * @brief 构造函数，用于创建 Vulkan 描述符池对象
         * @param device 指向 Vulkan 设备对象的指针，用于获取逻辑设备句柄
         * @param maxSets 描述符池可以分配的最大描述符集数量
         * @param poolSizes 描述符池中每种类型描述符的数量配置
         */
        AdVKDescriptorPool::AdVKDescriptorPool(AdVKDevice* device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes) : mDevice(device) {
                // 填充描述符池创建信息结构体
                VkDescriptorPoolCreateInfo descriptorPoolInfo = {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                        .maxSets = maxSets,
                        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
                        .pPoolSizes = poolSizes.data()
                };
                // 调用 Vulkan API 创建描述符池，并检查结果
                CALL_VK(vkCreateDescriptorPool(mDevice->GetHandle(), &descriptorPoolInfo, nullptr, &mHandle));
        }

        /**
         * @brief 析构函数，用于销毁 Vulkan 描述符池对象
         */
        AdVKDescriptorPool::~AdVKDescriptorPool() {
                // 使用宏 VK_D 销毁描述符池对象
                VK_D(DescriptorPool, mDevice->GetHandle(), mHandle);
        }

        /**
         * @brief 从描述符池中分配指定数量的描述符集
         * @param setLayout 描述符集布局对象指针，用于指定要分配的描述符集的布局
         * @param count 要分配的描述符集数量
         * @return 成功时返回包含分配的描述符集句柄的向量，失败时返回空向量
         */
        std::vector<VkDescriptorSet> AdVKDescriptorPool::AllocateDescriptorSet(AdVKDescriptorSetLayout* setLayout, uint32_t count) {
                // 初始化描述符集句柄向量和布局句柄向量
                std::vector<VkDescriptorSet> descriptorSets(count);
                std::vector<VkDescriptorSetLayout> setLayouts(count);
                // 填充布局句柄向量，所有描述符集使用相同的布局
                for (int i = 0; i < count; i++) {
                        setLayouts[i] = setLayout->GetHandle();
                }
                // 填充描述符集分配信息结构体
                VkDescriptorSetAllocateInfo allocateInfo = {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                        .pNext = nullptr,
                        .descriptorPool = mHandle,
                        .descriptorSetCount = count,
                        .pSetLayouts = setLayouts.data()
                };
                // 调用 Vulkan API 分配描述符集
                VkResult ret = vkAllocateDescriptorSets(mDevice->GetHandle(), &allocateInfo, descriptorSets.data());
                CALL_VK(ret);
                // 如果分配失败，则清空返回的描述符集向量
                if (ret != VK_SUCCESS) {
                        descriptorSets.clear();
                }
                return descriptorSets;
        }
}