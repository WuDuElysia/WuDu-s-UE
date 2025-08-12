#include "Graphic/AdVKDescriptorSet.h"
#include "Graphic/AdVKDevice.h"

namespace ade {

        /**
         * @brief ���캯�������ڴ��� Vulkan �����������ֶ���
         * @param device ָ�� Vulkan �豸�����ָ�룬���ڻ�ȡ�߼��豸���
         * @param bindings �����������ְ���Ϣ�����飬������ÿ���󶨵����������͡������ͽ׶ε�
         */
        AdVKDescriptorSetLayout::AdVKDescriptorSetLayout(AdVKDevice* device, const std::vector<VkDescriptorSetLayoutBinding>& bindings) : mDevice(device) {
                // ��������������ִ�����Ϣ�ṹ��
                VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .bindingCount = static_cast<uint32_t>(bindings.size()),
                        .pBindings = bindings.data()
                };
                // ���� Vulkan API ���������������֣��������
                CALL_VK(vkCreateDescriptorSetLayout(mDevice->GetHandle(), &descriptorSetLayoutInfo, nullptr, &mHandle));
        }

        /**
         * @brief ������������������ Vulkan �����������ֶ���
         */
        AdVKDescriptorSetLayout::~AdVKDescriptorSetLayout() {
                // ʹ�ú� VK_D ���������������ֶ���
                VK_D(DescriptorSetLayout, mDevice->GetHandle(), mHandle);
        }

        /**
         * @brief ���캯�������ڴ��� Vulkan �������ض���
         * @param device ָ�� Vulkan �豸�����ָ�룬���ڻ�ȡ�߼��豸���
         * @param maxSets �������ؿ��Է�������������������
         * @param poolSizes ����������ÿ����������������������
         */
        AdVKDescriptorPool::AdVKDescriptorPool(AdVKDevice* device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes) : mDevice(device) {
                // ����������ش�����Ϣ�ṹ��
                VkDescriptorPoolCreateInfo descriptorPoolInfo = {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .maxSets = maxSets,
                        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
                        .pPoolSizes = poolSizes.data()
                };
                // ���� Vulkan API �����������أ��������
                CALL_VK(vkCreateDescriptorPool(mDevice->GetHandle(), &descriptorPoolInfo, nullptr, &mHandle));
        }

        /**
         * @brief ������������������ Vulkan �������ض���
         */
        AdVKDescriptorPool::~AdVKDescriptorPool() {
                // ʹ�ú� VK_D �����������ض���
                VK_D(DescriptorPool, mDevice->GetHandle(), mHandle);
        }

        /**
         * @brief �����������з���ָ����������������
         * @param setLayout �����������ֶ���ָ�룬����ָ��Ҫ��������������Ĳ���
         * @param count Ҫ�����������������
         * @return �ɹ�ʱ���ذ���������������������������ʧ��ʱ���ؿ�����
         */
        std::vector<VkDescriptorSet> AdVKDescriptorPool::AllocateDescriptorSet(AdVKDescriptorSetLayout* setLayout, uint32_t count) {
                // ��ʼ������������������Ͳ��־������
                std::vector<VkDescriptorSet> descriptorSets(count);
                std::vector<VkDescriptorSetLayout> setLayouts(count);
                // ��䲼�־��������������������ʹ����ͬ�Ĳ���
                for (int i = 0; i < count; i++) {
                        setLayouts[i] = setLayout->GetHandle();
                }
                // �����������������Ϣ�ṹ��
                VkDescriptorSetAllocateInfo allocateInfo = {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                        .pNext = nullptr,
                        .descriptorPool = mHandle,
                        .descriptorSetCount = count,
                        .pSetLayouts = setLayouts.data()
                };
                // ���� Vulkan API ������������
                VkResult ret = vkAllocateDescriptorSets(mDevice->GetHandle(), &allocateInfo, descriptorSets.data());
                CALL_VK(ret);
                // �������ʧ�ܣ�����շ��ص�������������
                if (ret != VK_SUCCESS) {
                        descriptorSets.clear();
                }
                return descriptorSets;
        }
}