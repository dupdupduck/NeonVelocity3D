#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <GLES2/gl2.h>
#include <android/log.h>
#include <string>

#define LOG_TAG "NeonVelocity"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace Shaders {

// 3D Cyber Shaders compatible with OpenGL ES 2.0 and 3.0
static const char* kVertexShaderSource =
    "attribute vec3 a_Position;\n"
    "attribute vec3 a_Normal;\n"
    "attribute vec4 a_Color;\n"
    "\n"
    "uniform mat4 u_MVP;\n"
    "uniform mat4 u_Model;\n"
    "uniform mat4 u_View;\n"
    "uniform float u_Time;\n"
    "uniform float u_Warp;\n"
    "\n"
    "varying vec3 v_FragPos;\n"
    "varying vec3 v_Normal;\n"
    "varying vec4 v_Color;\n"
    "varying float v_Depth;\n"
    "\n"
    "void main() {\n"
    "    vec3 pos = a_Position;\n"
    "    if (u_Warp > 0.0) {\n"
    "        pos.x += sin(pos.z * 0.08 + u_Time * 3.0) * u_Warp;\n"
    "        pos.y += cos(pos.z * 0.06 + u_Time * 2.5) * (u_Warp * 0.6);\n"
    "    }\n"
    "    vec4 worldPos = u_Model * vec4(pos, 1.0);\n"
    "    v_FragPos = worldPos.xyz;\n"
    "    v_Normal = mat3(u_Model[0].xyz, u_Model[1].xyz, u_Model[2].xyz) * a_Normal;\n"
    "    v_Color = a_Color;\n"
    "    vec4 viewPos = u_View * worldPos;\n"
    "    v_Depth = -viewPos.z;\n"
    "    gl_Position = u_MVP * vec4(pos, 1.0);\n"
    "}\n";

static const char* kFragmentShaderSource =
    "precision mediump float;\n"
    "\n"
    "varying vec3 v_FragPos;\n"
    "varying vec3 v_Normal;\n"
    "varying vec4 v_Color;\n"
    "varying float v_Depth;\n"
    "\n"
    "uniform vec3 u_CameraPos;\n"
    "uniform vec3 u_LightPos;\n"
    "uniform vec3 u_LightColor;\n"
    "uniform float u_Time;\n"
    "\n"
    "void main() {\n"
    "    vec3 N = normalize(v_Normal);\n"
    "    vec3 V = normalize(u_CameraPos - v_FragPos);\n"
    "    vec3 L = normalize(u_LightPos - v_FragPos);\n"
    "\n"
    "    // Ambient lighting\n"
    "    vec3 ambient = v_Color.rgb * 0.35;\n"
    "\n"
    "    // Diffuse lighting\n"
    "    float diff = max(dot(N, L), 0.0);\n"
    "    float dist = length(u_LightPos - v_FragPos);\n"
    "    float atten = 1.0 / (1.0 + 0.04 * dist + 0.005 * dist * dist);\n"
    "    vec3 diffuse = diff * u_LightColor * atten * 1.5;\n"
    "\n"
    "    // Specular\n"
    "    vec3 H = normalize(L + V);\n"
    "    float spec = pow(max(dot(N, H), 0.0), 16.0);\n"
    "    vec3 specular = spec * vec3(1.0) * atten * 0.8;\n"
    "\n"
    "    // Fresnel Rim Glow (Cyberpunk edge glow)\n"
    "    float fresnel = 1.0 - max(dot(V, N), 0.0);\n"
    "    fresnel = pow(fresnel, 2.0);\n"
    "    vec3 rim = v_Color.rgb * fresnel * 1.8;\n"
    "\n"
    "    // Combined surface color\n"
    "    vec3 litColor = ambient + diffuse + specular + rim;\n"
    "\n"
    "    // Exponential Fog (Deep Neon Purple & Cosmic Black)\n"
    "    float fogDensity = 0.018;\n"
    "    float fogFactor = 1.0 - exp(-pow(v_Depth * fogDensity, 2.0));\n"
    "    fogFactor = clamp(fogFactor, 0.0, 1.0);\n"
    "\n"
    "    // Dynamic horizon fog gradient\n"
    "    vec3 fogHorizon = vec3(0.05, 0.02, 0.12);\n"
    "    vec3 fogDistant = vec3(0.85, 0.1, 0.65);\n"
    "    vec3 fogColor = mix(fogHorizon, fogDistant, clamp(v_Depth * 0.015, 0.0, 1.0));\n"
    "\n"
    "    vec3 finalColor = mix(litColor, fogColor, fogFactor);\n"
    "    gl_FragColor = vec4(finalColor, v_Color.a);\n"
    "}\n";

// Particle 2D/3D Shader for thrusters and explosions
static const char* kParticleVertShader =
    "attribute vec3 a_Position;\n"
    "attribute vec4 a_Color;\n"
    "attribute float a_Size;\n"
    "\n"
    "uniform mat4 u_VP;\n"
    "\n"
    "varying vec4 v_Color;\n"
    "\n"
    "void main() {\n"
    "    v_Color = a_Color;\n"
    "    vec4 clipPos = u_VP * vec4(a_Position, 1.0);\n"
    "    gl_Position = clipPos;\n"
    "    gl_PointSize = clamp(a_Size * (150.0 / max(clipPos.w, 1.0)), 2.0, 60.0);\n"
    "}\n";

static const char* kParticleFragShader =
    "precision mediump float;\n"
    "varying vec4 v_Color;\n"
    "void main() {\n"
    "    vec2 coord = gl_PointCoord - vec2(0.5);\n"
    "    float distSq = dot(coord, coord);\n"
    "    if (distSq > 0.25) discard;\n"
    "    float alpha = smoothstep(0.25, 0.0, distSq) * v_Color.a;\n"
    "    gl_FragColor = vec4(v_Color.rgb * 2.0, alpha);\n"
    "}\n";

// Helper functions for shader compilation
inline GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0) {
            char* buf = new char[infoLen];
            glGetShaderInfoLog(shader, infoLen, nullptr, buf);
            LOGE("Shader compilation error: %s", buf);
            delete[] buf;
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

inline GLuint CreateProgram(const char* vertSource, const char* fragSource) {
    GLuint vert = CompileShader(GL_VERTEX_SHADER, vertSource);
    if (!vert) return 0;

    GLuint frag = CompileShader(GL_FRAGMENT_SHADER, fragSource);
    if (!frag) {
        glDeleteShader(vert);
        return 0;
    }

    GLuint program = glCreateProgram();
    if (!program) return 0;

    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0) {
            char* buf = new char[infoLen];
            glGetProgramInfoLog(program, infoLen, nullptr, buf);
            LOGE("Program linking error: %s", buf);
            delete[] buf;
        }
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

} // namespace Shaders

#endif // SHADERS_HPP
