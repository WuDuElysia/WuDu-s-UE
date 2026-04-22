#version 450

layout(location=0) in vec3 v_LocalPos;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main() {
    // The sample direction is the interpolated local position (normalized)
    vec3 N = normalize(v_LocalPos);

    vec3 irradiance = vec3(0.0);

    // Build a tangent-space basis around N
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));

    // Convolve the environment map over the hemisphere
    // Using uniform sampling with fixed step sizes
    float sampleDelta = 0.025;
    float nrSamples = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            // Spherical to cartesian (tangent space)
            vec3 tangentSample = vec3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );

            // Tangent space to world space
            vec3 sampleVec = tangentSample.x * right
                           + tangentSample.y * up
                           + tangentSample.z * N;

            irradiance += texture(environmentMap, sampleVec).rgb
                        * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    outColor = vec4(irradiance, 1.0);
}
