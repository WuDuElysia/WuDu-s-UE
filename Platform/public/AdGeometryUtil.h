#ifndef ADGEOMETRYUTIL_H
#define ADGEOMETRYUTIL_H

#include "AdGraphicContext.h"

namespace WuDu {
        struct AdVertex {
                glm::vec3 pos;
                glm::vec2 tex;
                glm::vec3 nor;
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

        };
}
#endif