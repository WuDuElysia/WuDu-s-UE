#include "AdGeometryUtil.h"

namespace WuDu {
        void AdGeometryUtil::CreateCube(float leftPlane, float rightPlane, float bottomPlane, float topPlane, float nearPlane, float farPlane,
                std::vector<AdVertex>& vertices, std::vector<uint32_t>& indices, const bool bUseTextcoords,
                const bool bUseNormals, const glm::mat4& relativeMat) {
                glm::mat4 normalMat = glm::transpose(glm::inverse(relativeMat));
                //    v6----- v5
                //   /|      /|
                //  v1------v0|
                //  | |     | |
                //  | |v7---|-|v4
                //  |/      |/
                //  v2------v3
                vertices = {
                        // v0-v1-v2-v3 front
                        { relativeMat * glm::vec4(rightPlane, topPlane, nearPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, topPlane, nearPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, bottomPlane, nearPlane, 1.f) },
                        { relativeMat * glm::vec4(rightPlane, bottomPlane, nearPlane, 1.f) },
                        // v0-v3-v4-v5 right
                        { relativeMat * glm::vec4(rightPlane, topPlane, nearPlane, 1.f) },
                        { relativeMat * glm::vec4(rightPlane, bottomPlane, nearPlane, 1.f) },
                        { relativeMat * glm::vec4(rightPlane, bottomPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(rightPlane, topPlane, farPlane, 1.f) },
                        // v0-v5-v6-v1 up
                        { relativeMat * glm::vec4(rightPlane, topPlane, nearPlane, 1.f) },
                        { relativeMat * glm::vec4(rightPlane, topPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, topPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, topPlane, nearPlane, 1.f) },
                        // v1-v6-v7-v2 left
                        { relativeMat * glm::vec4(leftPlane, topPlane, nearPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, topPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, bottomPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, bottomPlane, nearPlane, 1.f) },
                        // v7-v4-v3-v2 bottom
                        { relativeMat * glm::vec4(leftPlane, bottomPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(rightPlane, bottomPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(rightPlane, bottomPlane, nearPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, bottomPlane, nearPlane, 1.f) },
                        // v4-v7-v6-v5 back
                        { relativeMat * glm::vec4(rightPlane, bottomPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, bottomPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(leftPlane, topPlane, farPlane, 1.f) },
                        { relativeMat * glm::vec4(rightPlane, topPlane, farPlane, 1.f) }
                };

                if (bUseTextcoords) {
                        vertices[0].tex = glm::vec2(0.0f, 0.0f);
                        vertices[1].tex = glm::vec2(1.0f, 0.0f);
                        vertices[2].tex = glm::vec2(1.0f, 1.0f);
                        vertices[3].tex = glm::vec2(0.0f, 1.0f);

                        vertices[4].tex = glm::vec2(1.0f, 0.0f);
                        vertices[5].tex = glm::vec2(1.0f, 1.0f);
                        vertices[6].tex = glm::vec2(0.0f, 1.0f);
                        vertices[7].tex = glm::vec2(0.0f, 0.0f);

                        vertices[8].tex = glm::vec2(1.0f, 0.0f);
                        vertices[9].tex = glm::vec2(1.0f, 1.0f);
                        vertices[10].tex = glm::vec2(0.0f, 1.0f);
                        vertices[11].tex = glm::vec2(0.0f, 0.0f);

                        vertices[12].tex = glm::vec2(0.0f, 0.0f);
                        vertices[13].tex = glm::vec2(1.0f, 0.0f);
                        vertices[14].tex = glm::vec2(1.0f, 1.0f);
                        vertices[15].tex = glm::vec2(0.0f, 1.0f);

                        vertices[16].tex = glm::vec2(0.0f, 0.0f);
                        vertices[17].tex = glm::vec2(1.0f, 0.0f);
                        vertices[18].tex = glm::vec2(1.0f, 1.0f);
                        vertices[19].tex = glm::vec2(0.0f, 1.0f);

                        vertices[20].tex = glm::vec2(1.0f, 1.0f);
                        vertices[21].tex = glm::vec2(0.0f, 1.0f);
                        vertices[22].tex = glm::vec2(0.0f, 0.0f);
                        vertices[23].tex = glm::vec2(1.0f, 0.0f);
                }

                if (bUseNormals) {
                        vertices[0].nor = normalMat * glm::vec4(0, 0, 1, 1);
                        vertices[1].nor = normalMat * glm::vec4(0, 0, 1, 1);
                        vertices[2].nor = normalMat * glm::vec4(0, 0, 1, 1);
                        vertices[3].nor = normalMat * glm::vec4(0, 0, 1, 1);
                        vertices[4].nor = normalMat * glm::vec4(1, 0, 0, 1);
                        vertices[5].nor = normalMat * glm::vec4(1, 0, 0, 1);
                        vertices[6].nor = normalMat * glm::vec4(1, 0, 0, 1);
                        vertices[7].nor = normalMat * glm::vec4(1, 0, 0, 1);
                        vertices[8].nor = normalMat * glm::vec4(0, 1, 0, 1);
                        vertices[9].nor = normalMat * glm::vec4(0, 1, 0, 1);
                        vertices[10].nor = normalMat * glm::vec4(0, 1, 0, 1);
                        vertices[11].nor = normalMat * glm::vec4(0, 1, 0, 1);
                        vertices[12].nor = normalMat * glm::vec4(-1, 0, 0, 1);
                        vertices[13].nor = normalMat * glm::vec4(-1, 0, 0, 1);
                        vertices[14].nor = normalMat * glm::vec4(-1, 0, 0, 1);
                        vertices[15].nor = normalMat * glm::vec4(-1, 0, 0, 1);
                        vertices[16].nor = normalMat * glm::vec4(0, -1, 0, 1);
                        vertices[17].nor = normalMat * glm::vec4(0, -1, 0, 1);
                        vertices[18].nor = normalMat * glm::vec4(0, -1, 0, 1);
                        vertices[19].nor = normalMat * glm::vec4(0, -1, 0, 1);
                        vertices[20].nor = normalMat * glm::vec4(0, 0, -1, 1);
                        vertices[21].nor = normalMat * glm::vec4(0, 0, -1, 1);
                        vertices[22].nor = normalMat * glm::vec4(0, 0, -1, 1);
                        vertices[23].nor = normalMat * glm::vec4(0, 0, -1, 1);
                }

                indices = {
                        0, 1, 2, 0, 2, 3,
                        4, 5, 6, 4, 6, 7,
                        8, 9, 10, 8, 10, 11,
                        12, 13, 14, 12, 14, 15,
                        16, 17, 18, 16, 18, 19,
                        20, 21, 22, 20, 22, 23
                };
        }
}