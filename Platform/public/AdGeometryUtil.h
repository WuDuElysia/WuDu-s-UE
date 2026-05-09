#ifndef ADGEOMETRYUTIL_H
#define ADGEOMETRYUTIL_H

#include "AdGraphicContext.h"

namespace WuDu {
        struct AdVertex {
                glm::vec3 Position;
                glm::vec3 Normal;
                glm::vec2 TexCoord;
                //切线
                glm::vec3 Tangent;
                //副法线
                glm::vec3 Bitangent;
        };

        class AdGeometryUtil {
        public:
                /**
                 * Create cube
                 * @param leftPlane   left
                 * @param rightPlane  right
                 * @param bottomPlane bottom
                 * @param topPlane    top
                 * @param nearPlane   near
                 * @param farPlane    far
                 * @param vertices    out vertices
                 * @param indices     out indices
                 * @param bUseTextcoords
                 * @param bUseNormals
                 */
                static void CreateCube(float leftPlane, float rightPlane, float bottomPlane, float topPlane, float nearPlane, float farPlane,
                        std::vector<AdVertex>& vertices, std::vector<uint32_t>& indices, const bool bUseTextcoords = true,
                        const bool bUseNormals = true, const glm::mat4& relativeMat = glm::mat4(1.0f));

                /**
                 * Create sphere (UV sphere)
                 * @param radius      球体半径
                 * @param sectors     经线细分数（水平方向）
                 * @param stacks      纬线细分数（垂直方向）
                 * @param vertices    out vertices
                 * @param indices     out indices
                 */
                static void CreateSphere(float radius, uint32_t sectors, uint32_t stacks,
                        std::vector<AdVertex>& vertices, std::vector<uint32_t>& indices);

        };
}
#endif