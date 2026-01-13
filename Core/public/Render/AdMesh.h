#ifndef AD_MESH_H
#define AD_MESH_H

#include "Graphic/AdVKBuffer.h"
#include "AdGeometryUtil.h"
#include "Resource/AdModelResource.h"

namespace WuDu {
	class AdMesh {
	public:
		AdMesh(const std::vector<WuDu::AdVertex>& vertices, const std::vector<uint32_t>& indices = {});
		AdMesh(const std::vector<WuDu::ModelVertex>& vertices, const std::vector<uint32_t>& indices = {});
		~AdMesh();

		void Draw(VkCommandBuffer cmdBuffer);

	private:
		std::shared_ptr<AdVKBuffer> mVertexBuffer;
		std::shared_ptr<AdVKBuffer> mIndexBuffer;
		uint32_t mVertexCount;
		uint32_t mIndexCount;
	};
}
#endif