#include "Render/AdMaterial.h"

namespace WuDu {
	/// 全局静态材质工厂实例
	AdMaterialFactory AdMaterialFactory::s_MaterialFactory{};

	/**
	 * @brief �������Ƿ����ָ��ID������
	 * @param id ����ID
	 * @return ������ڷ���true�����򷵻�false
	 */
	bool AdMaterial::HasTexture(uint32_t id) const {
		// ��������ID�Ƿ����
		if (mTextures.find(id) != mTextures.end()) {
			return true;
		}
		return false;
	}

	/**
	 * @brief ��ȡָ��ID��������ͼָ��
	 * @param id ����ID
	 * @return ������ͼָ�룬��������ڷ���nullptr
	 */
	const TextureView* AdMaterial::GetTextureView(uint32_t id) const {
		// ��������Ƿ���ڲ����ض�Ӧ��ͼ
		if (HasTexture(id)) {
			return &mTextures.at(id);
		}
		return nullptr;
	}

	/**
	 * @brief ����ָ��ID��������ͼ
	 * @param id ����ID
	 * @param texture ��������ָ��
	 * @param sampler ����������ָ��
	 */
	void AdMaterial::SetTextureView(uint32_t id, AdTexture* texture, AdSampler* sampler) {
		// ��������Ѵ�������£����򴴽���������ͼ
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
	 * @brief ����ָ��ID������ͼ������״̬
	 * @param id ����ID
	 * @param enable ����״̬
	 */
	void AdMaterial::UpdateTextureViewEnable(uint32_t id, bool enable) {
		// ������������״̬����ǲ�����Ҫˢ��
		if (HasTexture(id)) {
			mTextures[id].bEnable = enable;
			bShouldFlushParams = true;
		}
	}

	/**
	 * @brief ����ָ��ID������ͼ��UVƽ�Ʋ���
	 * @param id ����ID
	 * @param uvTranslation UVƽ������
	 */
	void AdMaterial::UpdateTextureViewUVTranslation(uint32_t id, const glm::vec2& uvTranslation) {
		// ��������UVƽ�Ʋ�������ǲ�����Ҫˢ��
		if (HasTexture(id)) {
			mTextures[id].uvTranslation = uvTranslation;
			bShouldFlushParams = true;
		}
	}

	/**
	 * @brief ����ָ��ID������ͼ��UV��ת����
	 * @param id ����ID
	 * @param uvRotation UV��ת�Ƕ�
	 */
	void AdMaterial::UpdateTextureViewUVRotation(uint32_t id, float uvRotation) {
		// ��������UV��ת��������ǲ�����Ҫˢ��
		if (HasTexture(id)) {
			mTextures[id].uvRotation = uvRotation;
			bShouldFlushParams = true;
		}
	}

	/**
	 * @brief ����ָ��ID������ͼ��UV���Ų���
	 * @param id ����ID
	 * @param uvScale UV��������
	 */
	void AdMaterial::UpdateTextureViewUVScale(uint32_t id, const glm::vec2& uvScale) {
		// ��������UV���Ų�������ǲ�����Ҫˢ��
		if (HasTexture(id)) {
			mTextures[id].uvScale = uvScale;
			bShouldFlushParams = true;
		}
	}
}