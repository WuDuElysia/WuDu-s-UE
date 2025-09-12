### 修改计划

**目标**：将原来使用 `glslangValidator` 的SPIR-V编译流程替换为使用Vulkan SDK中的 `glslc` 工具。

**修改文件**：`f:/VS2022/WuDu's UE/cmake/SPIR-V.cmake`

**修改内容**：
1. **替换工具查找**：将 `find_program(GLSLANGVALIDATOR_COMMAND glslangValidator)` 替换为查找 `glslc`
2. **更新工具变量名**：将所有 `GLSLANGVALIDATOR_COMMAND` 替换为 `GLSLC_COMMAND`
3. **修改编译命令**：将 `glslangValidator` 的参数替换为 `glslc` 的参数
   - 原命令：`glslangValidator -V --target-env spirv1.0 ${GLSL} -o ${HEADER}`
   - 新命令：`glslc ${GLSL} -o ${HEADER}`
4. **保持其他逻辑不变**：确保生成的文件路径、依赖关系和函数返回值保持一致

**修改后的函数流程**：
1. 遍历指定的着色器源文件
2. 为每个着色器文件生成对应的SPIR-V二进制文件
3. 输出路径：`${AD_DEFINE_RES_ROOT_DIR}Shader/${GLSL}.spv`
4. 使用 `glslc` 工具将GLSL编译为SPIR-V
5. 生成自定义构建命令，确保着色器在源文件变化时重新编译

**预期效果**：
- 保持原有的CMake函数接口不变，确保现有项目可以无缝迁移
- 使用Vulkan SDK自带的 `glslc` 工具替代 `glslangValidator`
- 简化编译命令，提高编译效率
- 与Vulkan SDK更好地集成

**注意事项**：
- 确保Vulkan SDK已正确安装，且 `glslc` 工具在系统PATH中
- 保持生成的SPIR-V文件路径不变，确保运行时资源加载正常
- 验证修改后所有示例项目仍能正常构建和运行