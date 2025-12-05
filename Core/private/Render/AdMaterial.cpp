#include "Render/AdMaterial.h"

namespace WuDu {
	/// 全局静态材质工厂实例
	AdMaterialFactory AdMaterialFactory::s_MaterialFactory{};

	/**
	 * @brief 检查材质是否包含指定ID的纹理
	 * @param id 纹理ID
	 * @return 如果存在返回true，否则返回false
	 */
	bool AdMaterial::HasTexture(uint32_t id) const {
		// 查找纹理ID是否存在
		if (mTextures.find(id) != mTextures.end()) {
			return true;
		}
		return false;
	}

	/**
	 * @brief 获取指定ID纹理的视图指针
	 * @param id 纹理ID
	 * @return 纹理视图指针，如果不存在返回nullptr
	 */
	const TextureView* AdMaterial::GetTextureView(uint32_t id) const {
		// 检查纹理是否存在并返回对应视图
		if (HasTexture(id)) {
			return &mTextures.at(id);
		}
		return nullptr;
	}

	/**
	 * @brief 设置指定ID的纹理视图
	 * @param id 纹理ID
	 * @param texture 纹理对象指针
	 * @param sampler 采样器对象指针
	 */
	void AdMaterial::SetTextureView(uint32_t id, AdTexture* texture, AdSampler* sampler) {
		// 如果纹理已存在则更新，否则创建新纹理视图
		if (HasTexture(id)) {
			mTextures[id].texture = texture;
			mTextures[id].sampler = sampler;
		}
		else {
			mTextures[id] = { texture, sampler };
		}
		bShouldFlushResource = true;
	}

	/**
	 * @brief 更新指定ID纹理视图的启用状态
	 * @param id 纹理ID
	 * @param enable 启用状态
	 */
	void AdMaterial::UpdateTextureViewEnable(uint32_t id, bool enable) {
		// 更新纹理启用状态并标记参数需要刷新
		if (HasTexture(id)) {
			mTextures[id].bEnable = enable;
			bShouldFlushParams = true;
		}
	}

	/**
	 * @brief 更新指定ID纹理视图的UV平移参数
	 * @param id 纹理ID
	 * @param uvTranslation UV平移向量
	 */
	void AdMaterial::UpdateTextureViewUVTranslation(uint32_t id, const glm::vec2& uvTranslation) {
		// 更新纹理UV平移参数并标记参数需要刷新
		if (HasTexture(id)) {
			mTextures[id].uvTranslation = uvTranslation;
			bShouldFlushParams = true;
		}
	}

	/**
	 * @brief 更新指定ID纹理视图的UV旋转参数
	 * @param id 纹理ID
	 * @param uvRotation UV旋转角度
	 */
	void AdMaterial::UpdateTextureViewUVRotation(uint32_t id, float uvRotation) {
		// 更新纹理UV旋转参数并标记参数需要刷新
		if (HasTexture(id)) {
			mTextures[id].uvRotation = uvRotation;
			bShouldFlushParams = true;
		}
	}

	/**
	 * @brief 更新指定ID纹理视图的UV缩放参数
	 * @param id 纹理ID
	 * @param uvScale UV缩放向量
	 */
	void AdMaterial::UpdateTextureViewUVScale(uint32_t id, const glm::vec2& uvScale) {
		// 更新纹理UV缩放参数并标记参数需要刷新
		if (HasTexture(id)) {
			mTextures[id].uvScale = uvScale;
			bShouldFlushParams = true;
		}
	}
}