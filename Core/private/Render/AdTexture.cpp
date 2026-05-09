#include "Render/AdTexture.h"

#include "AdApplication.h"
#include "Render/AdRenderContext.h"
#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKImage.h"
#include "Graphic/AdVKImageView.h"
#include "Graphic/AdVKBuffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace WuDu {
	/**
	* @brief 构造函数，根据指定文件路径加载并创建纹理资源
	* @param filePath 纹理文件的路径
	*/
	AdTexture::AdTexture(const std::string& filePath) {
		LOG_D("[AdTexture] Constructor called with path: \"{0}\"", filePath);
		// 加载图像数据
		int numChannel;
		uint8_t* data = stbi_load(filePath.c_str(), reinterpret_cast<int*>(&mWidth), reinterpret_cast<int*>(&mHeight), &numChannel, STBI_rgb_alpha);
		if (!data) {
			LOG_E("Can not load this image: {0}", filePath);
			return;
		}
		LOG_D("[AdTexture] stbi_load OK: {0}x{1}, channels={2}", mWidth, mHeight, numChannel);

		// 设置图像格式并计算图像数据大小
		mFormat = VK_FORMAT_R8G8B8A8_UNORM;
		size_t size = sizeof(uint8_t) * 4 * mWidth * mHeight;

		// 创建纹理图像
		CreateImage(size, data);

		// 释放图像原始内存
		stbi_image_free(data);
	}

	/**
	* @brief AdTexture的构造函数，用于创建一个纯色纹理
	* @param width 纹理的宽度，像素
	* @param height 纹理的高度，像素
	* @param pixels 指向RGBA颜色数据的指针，用于初始化纹理数据
	*/
	AdTexture::AdTexture(uint32_t width, uint32_t height, RGBAColor* pixels) : mWidth(width), mHeight(height) {
		mFormat = VK_FORMAT_R8G8B8A8_UNORM;
		// 计算纹理数据的字节大小
		size_t size = sizeof(uint8_t) * 4 * mWidth * mHeight;
		// 创建纹理资源并初始化纹理数据
		CreateImage(size, pixels);
	}

	AdTexture::~AdTexture() {
		mImageView.reset();
		mImage.reset();
	}

	/**
	* @brief 创建纹理图像并上传数据
	* @param size 图像数据的大小(字节)
	* @param data 指向图像数据的指针
	*
	* 该方法负责创建Vulkan图像资源、图像视图和采样器，并将图像数据上传到GPU内存中
	* 管理图像布局转换和数据传输到纹理内存
	*/
	void AdTexture::CreateImage(size_t size, void* data) {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();

		LOG_D("[AdTexture::CreateImage] Start. size={0}, width={1}, height={2}", size, mWidth, mHeight);

		// 创建Vulkan图像和图像视图资源
		mImage = std::make_shared<AdVKImage>(device, VkExtent3D{ mWidth, mHeight, 1 }, mFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT);
		mImageView = std::make_shared<AdVKImageView>(device, mImage->GetHandle(), mFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		LOG_D("[AdTexture::CreateImage] VkImage and VkImageView created");

		// 创建临时数据缓冲区用于数据传输
		std::shared_ptr<AdVKBuffer> stageBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, data, true);
		LOG_D("[AdTexture::CreateImage] Staging buffer created");

		// 执行图像布局转换和数据传输操作
		LOG_D("[AdTexture::CreateImage] Calling CreateAndBeginOneCmdBuffer...");
		VkCommandBuffer cmdBuffer = device->CreateAndBeginOneCmdBuffer();
		LOG_D("[AdTexture::CreateImage] CmdBuffer created: {0}", (void*)cmdBuffer);

		// 将图像布局从VK_IMAGE_LAYOUT_UNDEFINED转换为VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL，为数据传输做准备
		AdVKImage::TransitionLayout(cmdBuffer, mImage->GetHandle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		LOG_D("[AdTexture::CreateImage] TransitionLayout UNDEFINED -> TRANSFER_DST done");

		// 将数据从临时缓冲区复制到图像
		mImage->CopyFromBuffer(cmdBuffer, stageBuffer.get());
		LOG_D("[AdTexture::CreateImage] CopyFromBuffer done");

		// 将图像布局从VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL转换为VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL，供着色器读取使用
		AdVKImage::TransitionLayout(cmdBuffer, mImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		LOG_D("[AdTexture::CreateImage] TransitionLayout TRANSFER_DST -> SHADER_READ_ONLY done");

		LOG_D("[AdTexture::CreateImage] Calling SubmitOneCmdBuffer...");
		device->SubmitOneCmdBuffer(cmdBuffer);
		LOG_D("[AdTexture::CreateImage] SubmitOneCmdBuffer returned OK");

		// 释放临时缓冲区
		stageBuffer.reset();
		LOG_D("[AdTexture::CreateImage] Done. Staging buffer released");
	}
}