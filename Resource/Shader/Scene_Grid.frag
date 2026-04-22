#version 450

// 从顶点着色器输入的近/远平面世界坐标
layout(location=0) in vec3 v_nearPos;
layout(location=1) in vec3 v_farPos;

// 输出颜色（alpha 混合）
layout(location=0) out vec4 outColor;

// 帧 UBO (Set 0, Binding 0)
layout(set=0, binding=0) uniform FrameUbo {
    mat4  projMat;
    mat4  viewMat;
    vec3  camPos;
    float _pad0;
    ivec2 resolution;
    uint  frameId;
    float time;
} frameUbo;

// ============================================================
// 网格参数常量
// ============================================================
const float GRID_SPACING      = 1.0;    // 次网格线基础间距（世界单位）
const int   MAJOR_GRID_MULT   = 10;     // 主网格线间距倍数
const vec4  GRID_COLOR        = vec4(0.5, 0.5, 0.5, 1.0);  // 普通网格线颜色
const vec4  X_AXIS_COLOR      = vec4(1.0, 0.0, 0.0, 1.0);  // X 轴颜色（红色）
const vec4  Z_AXIS_COLOR      = vec4(0.0, 0.0, 1.0, 1.0);  // Z 轴颜色（蓝色）
const float FADE_DISTANCE     = 100.0;  // 网格线完全淡出距离
const float MINOR_FADE_FACTOR = 0.5;    // 次网格线衰减速率系数

// ============================================================
// 网格线检测：使用 fwidth 实现抗锯齿
// ============================================================
float gridLine(float coord, float spacing) {
    // abs(fract(coord/spacing - 0.5) - 0.5) / fwidth(coord/spacing)
    float scaledCoord = coord / spacing;
    float line = abs(fract(scaledCoord - 0.5) - 0.5) / fwidth(scaledCoord);
    return 1.0 - min(line, 1.0);
}

// ============================================================
// 主函数
// ============================================================
void main() {
    // ----------------------------------------------------------
    // 1. 射线-平面求交：从 v_nearPos 到 v_farPos 与 Y=0 平面
    // ----------------------------------------------------------
    float t = -v_nearPos.y / (v_farPos.y - v_nearPos.y);

    // 丢弃不与 XZ 平面相交的片段
    if (t < 0.0 || t > 1.0) {
        discard;
    }

    // 计算 XZ 平面上的世界坐标
    vec3 worldPos = v_nearPos + t * (v_farPos - v_nearPos);

    // ----------------------------------------------------------
    // 2. 双层级网格线检测
    // ----------------------------------------------------------
    float minorSpacing = GRID_SPACING;
    float majorSpacing = GRID_SPACING * float(MAJOR_GRID_MULT);

    // 次网格线（X 方向和 Z 方向取最大值）
    float minorLineX = gridLine(worldPos.x, minorSpacing);
    float minorLineZ = gridLine(worldPos.z, minorSpacing);
    float minorLine  = max(minorLineX, minorLineZ);

    // 主网格线
    float majorLineX = gridLine(worldPos.x, majorSpacing);
    float majorLineZ = gridLine(worldPos.z, majorSpacing);
    float majorLine  = max(majorLineX, majorLineZ);

    // ----------------------------------------------------------
    // 3. 坐标轴颜色指示
    // ----------------------------------------------------------
    float axisLineWidth = fwidth(worldPos.x) * 1.5;

    // |z| < threshold → 红色（X 轴方向）
    float xAxisLine = 1.0 - min(abs(worldPos.z) / axisLineWidth, 1.0);
    // |x| < threshold → 蓝色（Z 轴方向）
    float zAxisLine = 1.0 - min(abs(worldPos.x) / axisLineWidth, 1.0);

    // ----------------------------------------------------------
    // 4. 距离衰减
    // ----------------------------------------------------------
    float dist = length(worldPos - frameUbo.camPos);
    float fadeFactor = 1.0 - clamp(dist / FADE_DISTANCE, 0.0, 1.0);

    // 次网格线衰减更快
    float minorFade = pow(fadeFactor, 1.0 / MINOR_FADE_FACTOR);
    // 主网格线正常衰减
    float majorFade = fadeFactor;
    // 坐标轴指示器衰减最慢
    float axisFade  = pow(fadeFactor, MINOR_FADE_FACTOR);

    // ----------------------------------------------------------
    // 5. 合成颜色和 alpha
    // ----------------------------------------------------------
    vec4 color = vec4(0.0);

    // 从底层到顶层叠加：次网格 → 主网格 → 坐标轴
    // 次网格线
    color = mix(color, GRID_COLOR, minorLine * minorFade);
    // 主网格线覆盖次网格线
    color = mix(color, GRID_COLOR, majorLine * majorFade);
    // X 轴（红色）覆盖
    color = mix(color, X_AXIS_COLOR, xAxisLine * axisFade);
    // Z 轴（蓝色）覆盖
    color = mix(color, Z_AXIS_COLOR, zAxisLine * axisFade);

    // 最终 alpha：取所有层级中最大的可见度
    float alpha = max(minorLine * minorFade,
                  max(majorLine * majorFade,
                  max(xAxisLine * axisFade,
                      zAxisLine * axisFade)));

    // 丢弃完全透明的片段
    if (alpha < 0.001) {
        discard;
    }

    outColor = vec4(color.rgb, alpha);

    // ----------------------------------------------------------
    // 6. 深度写入：将 XZ 平面交点重新投影到裁剪空间
    // ----------------------------------------------------------
    vec4 clipPos = frameUbo.projMat * frameUbo.viewMat * vec4(worldPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;
}
