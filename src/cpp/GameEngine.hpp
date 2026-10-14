#ifndef GAMEENGINE_HPP
#define GAMEENGINE_HPP

#include "Math3D.hpp"
#include "Shaders.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>

namespace Game {

using namespace Math3D;

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b, a;
};

struct Particle {
    Vector3 pos;
    Vector3 vel;
    Vector4 color;
    float size;
    float life;
    float maxLife;
};

struct Obstacle {
    Vector3 pos;
    Vector3 size;
    Vector3 rotation;
    Vector3 rotSpeed;
    Vector4 color;
    bool active = true;
};

struct Shard {
    Vector3 pos;
    float rotY = 0.0f;
    Vector4 color = {1.0f, 0.85f, 0.1f, 1.0f}; // Neon Gold
    bool active = true;
};

class GameEngine {
public:
    int screenWidth = 1080;
    int screenHeight = 2400;

    // OpenGL handles
    GLuint mainProgram = 0;
    GLuint particleProgram = 0;

    // Uniform locations
    GLint u_MVP = -1;
    GLint u_Model = -1;
    GLint u_View = -1;
    GLint u_Time = -1;
    GLint u_Warp = -1;
    GLint u_CameraPos = -1;
    GLint u_LightPos = -1;
    GLint u_LightColor = -1;

    GLint u_PartVP = -1;

    // Attribute locations
    GLint a_Position = -1;
    GLint a_Normal = -1;
    GLint a_Color = -1;

    GLint a_PartPos = -1;
    GLint a_PartColor = -1;
    GLint a_PartSize = -1;

    // Game Objects & State
    Vector3 playerPos = {0.0f, 0.0f, 0.0f};
    Vector3 playerTargetPos = {0.0f, 0.0f, 0.0f};
    float playerTilt = 0.0f;
    float speed = 25.0f;
    float distance = 0.0f;
    int score = 0;
    int shardsCollected = 0;
    int health = 3;
    bool isGameOver = false;

    float gameTime = 0.0f;
    float cameraShake = 0.0f;
    float nearMissTimer = 0.0f;

    // Mesh Buffers
    std::vector<Vertex> tunnelVertices;
    std::vector<Vertex> shipVertices;
    std::vector<Vertex> cubeVertices;
    std::vector<Vertex> shardVertices;

    // Active World Entities
    std::vector<Obstacle> obstacles;
    std::vector<Shard> shards;
    std::vector<Particle> particles;

    // Touch input state
    float touchX = 0.0f;
    float touchY = 0.0f;
    bool isTouching = false;

    GameEngine() {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    void InitGL(int width, int height) {
        screenWidth = width;
        screenHeight = height;

        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Compile Shader Programs
        mainProgram = Shaders::CreateProgram(Shaders::kVertexShaderSource, Shaders::kFragmentShaderSource);
        particleProgram = Shaders::CreateProgram(Shaders::kParticleVertShader, Shaders::kParticleFragShader);

        // Query Uniforms
        u_MVP = glGetUniformLocation(mainProgram, "u_MVP");
        u_Model = glGetUniformLocation(mainProgram, "u_Model");
        u_View = glGetUniformLocation(mainProgram, "u_View");
        u_Time = glGetUniformLocation(mainProgram, "u_Time");
        u_Warp = glGetUniformLocation(mainProgram, "u_Warp");
        u_CameraPos = glGetUniformLocation(mainProgram, "u_CameraPos");
        u_LightPos = glGetUniformLocation(mainProgram, "u_LightPos");
        u_LightColor = glGetUniformLocation(mainProgram, "u_LightColor");

        a_Position = glGetAttribLocation(mainProgram, "a_Position");
        a_Normal = glGetAttribLocation(mainProgram, "a_Normal");
        a_Color = glGetAttribLocation(mainProgram, "a_Color");

        u_PartVP = glGetUniformLocation(particleProgram, "u_VP");
        a_PartPos = glGetAttribLocation(particleProgram, "a_Position");
        a_PartColor = glGetAttribLocation(particleProgram, "a_Color");
        a_PartSize = glGetAttribLocation(particleProgram, "a_Size");

        // Build 3D Geometric Meshes
        BuildTunnelMesh();
        BuildShipMesh();
        BuildCubeMesh();
        BuildShardMesh();

        ResetGame();
        LOGI("Game Engine GL Initialized successfully (%dx%d)", width, height);
    }

    void ResetGame() {
        playerPos = {0.0f, 0.0f, 0.0f};
        playerTargetPos = {0.0f, 0.0f, 0.0f};
        playerTilt = 0.0f;
        speed = 28.0f;
        distance = 0.0f;
        score = 0;
        shardsCollected = 0;
        health = 3;
        isGameOver = false;
        cameraShake = 0.0f;
        gameTime = 0.0f;

        obstacles.clear();
        shards.clear();
        particles.clear();

        // Spawn initial wave of obstacles and shards
        for (float z = 40.0f; z < 400.0f; z += 25.0f) {
            SpawnWaveAt(z);
        }
    }

    void SpawnWaveAt(float z) {
        // Spawn 1 to 2 obstacles
        int lane = (std::rand() % 5) - 2; // -2, -1, 0, 1, 2
        float posX = lane * 1.8f;
        float posY = ((std::rand() % 3) - 1) * 0.8f;

        Obstacle obs;
        obs.pos = {posX, posY, z};
        obs.size = {1.2f, 1.2f, 1.2f};
        obs.rotation = {0.0f, 0.0f, 0.0f};
        obs.rotSpeed = {
            (std::rand() % 100 - 50) * 0.02f,
            (std::rand() % 100 - 50) * 0.03f,
            (std::rand() % 100 - 50) * 0.02f
        };
        // Alternating neon colors: Cyan, Neon Pink, Orange Laser
        int colorType = std::rand() % 3;
        if (colorType == 0) obs.color = {0.0f, 0.95f, 1.0f, 1.0f}; // Cyan
        else if (colorType == 1) obs.color = {1.0f, 0.05f, 0.6f, 1.0f}; // Hot Pink
        else obs.color = {1.0f, 0.35f, 0.0f, 1.0f}; // Orange

        obstacles.push_back(obs);

        // Spawn collectible shard in a clear lane
        if (std::rand() % 100 < 75) {
            int shardLane = (lane + 2) % 5 - 2;
            Shard s;
            s.pos = {shardLane * 1.8f, 0.0f, z + 12.0f};
            shards.push_back(s);
        }
    }

    void HandleTouchDown(float x, float y) {
        isTouching = true;
        touchX = x;
        touchY = y;

        if (isGameOver) {
            ResetGame();
        }
    }

    void HandleTouchMove(float x, float y) {
        if (!isTouching) return;
        touchX = x;
        touchY = y;

        // Map touch X from screen coordinates [0, screenWidth] to game lane range [-3.5, 3.5]
        float normX = (x / static_cast<float>(screenWidth)) * 2.0f - 1.0f; // [-1.0, 1.0]
        playerTargetPos.x = normX * 3.6f;
        playerTargetPos.x = std::clamp(playerTargetPos.x, -3.8f, 3.8f);

        // Subtle vertical control
        float normY = (y / static_cast<float>(screenHeight)) * 2.0f - 1.0f;
        playerTargetPos.y = -normY * 1.2f;
        playerTargetPos.y = std::clamp(playerTargetPos.y, -1.8f, 1.8f);
    }

    void HandleTouchUp() {
        isTouching = false;
    }

    void Update(float dt) {
        gameTime += dt;

        if (isGameOver) {
            UpdateParticles(dt);
            return;
        }

        // Gradually increase speed
        speed = std::min(75.0f, 28.0f + distance * 0.015f);
        distance += speed * dt;
        score = static_cast<int>(distance * 1.5f) + (shardsCollected * 100);

        // Smooth player position interpolation
        float smoothFactor = 12.0f * dt;
        playerPos.x += (playerTargetPos.x - playerPos.x) * std::clamp(smoothFactor, 0.0f, 1.0f);
        playerPos.y += (playerTargetPos.y - playerPos.y) * std::clamp(smoothFactor, 0.0f, 1.0f);
        playerPos.z = distance;

        // Realistic banking tilt when moving
        float targetTilt = (playerTargetPos.x - playerPos.x) * -0.35f;
        playerTilt += (targetTilt - playerTilt) * (15.0f * dt);

        // Spawn engine thruster particles
        SpawnThrusterParticles();

        // Update Obstacles
        for (auto& obs : obstacles) {
            if (!obs.active) continue;
            obs.rotation += obs.rotSpeed * dt;

            // Collision check (Sphere-AABB simplified)
            float dx = std::abs(obs.pos.x - playerPos.x);
            float dy = std::abs(obs.pos.y - playerPos.y);
            float dz = std::abs(obs.pos.z - playerPos.z);

            if (dx < 0.95f && dy < 0.95f && dz < 1.1f) {
                // Collision!
                health--;
                cameraShake = 1.0f;
                obs.active = false;
                SpawnExplosion(obs.pos, obs.color, 40);

                if (health <= 0) {
                    isGameOver = true;
                    SpawnExplosion(playerPos, {1.0f, 0.2f, 0.1f, 1.0f}, 80);
                    break;
                }
            } else if (dz < 2.0f && (dx < 1.6f || dy < 1.6f)) {
                // Near-miss grazing bonus
                score += 5;
                nearMissTimer = 0.2f;
            }
        }

        // Update Collectible Shards
        for (auto& s : shards) {
            if (!s.active) continue;
            s.rotY += 3.5f * dt;

            float distSq = (s.pos - playerPos).LengthSq();
            if (distSq < 1.8f) {
                s.active = false;
                shardsCollected++;
                score += 100;
                SpawnExplosion(s.pos, s.color, 25);
            }
        }

        // Spawn new waves as player travels forward
        float maxSpawnZ = playerPos.z + 350.0f;
        float lastObsZ = obstacles.empty() ? playerPos.z : obstacles.back().pos.z;
        if (lastObsZ < maxSpawnZ) {
            SpawnWaveAt(lastObsZ + 22.0f);
        }

        // Cleanup passed objects
        while (!obstacles.empty() && obstacles.front().pos.z < playerPos.z - 20.0f) {
            obstacles.erase(obstacles.begin());
        }
        while (!shards.empty() && shards.front().pos.z < playerPos.z - 20.0f) {
            shards.erase(shards.begin());
        }

        // Decay camera shake
        if (cameraShake > 0.0f) {
            cameraShake = std::max(0.0f, cameraShake - dt * 3.0f);
        }

        UpdateParticles(dt);
    }

    void SpawnThrusterParticles() {
        for (int i = 0; i < 2; ++i) {
            float side = (i == 0) ? -0.42f : 0.42f;
            Particle p;
            p.pos = playerPos + Vector3(side, -0.05f, -0.9f);
            p.vel = Vector3((std::rand() % 100 - 50) * 0.01f, (std::rand() % 100 - 50) * 0.01f, -15.0f - (std::rand() % 100) * 0.05f);
            p.color = {0.0f, 0.9f, 1.0f, 1.0f}; // Cyan plasma
            p.size = 14.0f + (std::rand() % 10);
            p.life = 0.25f;
            p.maxLife = 0.25f;
            particles.push_back(p);
        }
    }

    void SpawnExplosion(const Vector3& origin, const Vector4& color, int count) {
        for (int i = 0; i < count; ++i) {
            Particle p;
            p.pos = origin;
            float vx = (std::rand() % 200 - 100) * 0.08f;
            float vy = (std::rand() % 200 - 100) * 0.08f;
            float vz = (std::rand() % 200 - 100) * 0.08f;
            p.vel = {vx, vy, vz};
            p.color = color;
            p.size = 18.0f + (std::rand() % 15);
            p.life = 0.6f + (std::rand() % 100) * 0.005f;
            p.maxLife = p.life;
            particles.push_back(p);
        }
    }

    void UpdateParticles(float dt) {
        for (size_t i = 0; i < particles.size();) {
            particles[i].life -= dt;
            if (particles[i].life <= 0.0f) {
                particles[i] = particles.back();
                particles.pop_back();
            } else {
                particles[i].pos += particles[i].vel * dt;
                particles[i].color.w = particles[i].life / particles[i].maxLife;
                ++i;
            }
        }
    }

    void Render() {
        glClearColor(0.03f, 0.01f, 0.08f, 1.0f); // Deep Cyber Void
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
        float fov = 72.0f + (speed - 28.0f) * 0.2f; // Dynamic speed warp FOV
        Matrix4x4 proj = Matrix4x4::Perspective(fov * DEG2RAD, aspect, 0.2f, 350.0f);

        // Camera Follow with Trauma Shake
        Vector3 camTarget = playerPos + Vector3(0.0f, 0.6f, 8.0f);
        Vector3 camPos = playerPos + Vector3(0.0f, 2.4f, -6.8f);

        if (cameraShake > 0.0f) {
            float s = cameraShake * cameraShake * 0.35f;
            camPos.x += ((std::rand() % 100) * 0.02f - 1.0f) * s;
            camPos.y += ((std::rand() % 100) * 0.02f - 1.0f) * s;
        }

        Matrix4x4 view = Matrix4x4::LookAt(camPos, camTarget, {0.0f, 1.0f, 0.0f});

        glUseProgram(mainProgram);

        glUniform1f(u_Time, gameTime);
        glUniform3f(u_CameraPos, camPos.x, camPos.y, camPos.z);
        glUniform3f(u_LightPos, playerPos.x, playerPos.y, playerPos.z + 1.0f);
        glUniform3f(u_LightColor, 0.0f, 0.95f, 1.0f);
        glUniformMatrix4fv(u_View, 1, GL_FALSE, view.Data());

        // 1. Render Infinite Hexagonal Cyber Tunnel
        RenderTunnel(proj, view);

        // 2. Render Player Ship (if alive)
        if (!isGameOver) {
            RenderShip(proj, view);
        }

        // 3. Render Obstacles
        RenderObstacles(proj, view);

        // 4. Render Collectible Shards
        RenderShards(proj, view);

        // 5. Render Particle System (Additive Blending)
        RenderParticles(proj, view);
    }

    void RenderTunnel(const Matrix4x4& proj, const Matrix4x4& view) {
        // Repeat tunnel segments around player position
        float tunnelZ = std::floor(playerPos.z / 20.0f) * 20.0f;
        for (int i = -1; i < 16; ++i) {
            Matrix4x4 model = Matrix4x4::Translation({0.0f, 0.0f, tunnelZ + (i * 20.0f)});
            Matrix4x4 mvp = proj * view * model;

            glUniformMatrix4fv(u_MVP, 1, GL_FALSE, mvp.Data());
            glUniformMatrix4fv(u_Model, 1, GL_FALSE, model.Data());
            glUniform1f(u_Warp, 1.8f); // Active tunnel sine wave warp

            DrawMesh(tunnelVertices);
        }
    }

    void RenderShip(const Matrix4x4& proj, const Matrix4x4& view) {
        Matrix4x4 model = Matrix4x4::Translation(playerPos) *
                          Matrix4x4::RotationZ(playerTilt) *
                          Matrix4x4::RotationY(playerTilt * 0.25f);
        Matrix4x4 mvp = proj * view * model;

        glUniformMatrix4fv(u_MVP, 1, GL_FALSE, mvp.Data());
        glUniformMatrix4fv(u_Model, 1, GL_FALSE, model.Data());
        glUniform1f(u_Warp, 0.0f); // No warp on player ship

        DrawMesh(shipVertices);
    }

    void RenderObstacles(const Matrix4x4& proj, const Matrix4x4& view) {
        glUniform1f(u_Warp, 0.0f);
        for (const auto& obs : obstacles) {
            if (!obs.active) continue;
            Matrix4x4 model = Matrix4x4::Translation(obs.pos) *
                              Matrix4x4::RotationX(obs.rotation.x) *
                              Matrix4x4::RotationY(obs.rotation.y) *
                              Matrix4x4::RotationZ(obs.rotation.z) *
                              Matrix4x4::Scale(obs.size);
            Matrix4x4 mvp = proj * view * model;

            glUniformMatrix4fv(u_MVP, 1, GL_FALSE, mvp.Data());
            glUniformMatrix4fv(u_Model, 1, GL_FALSE, model.Data());

            DrawMeshWithColor(cubeVertices, obs.color);
        }
    }

    void RenderShards(const Matrix4x4& proj, const Matrix4x4& view) {
        glUniform1f(u_Warp, 0.0f);
        for (const auto& s : shards) {
            if (!s.active) continue;
            Matrix4x4 model = Matrix4x4::Translation(s.pos) *
                              Matrix4x4::RotationY(s.rotY) *
                              Matrix4x4::Scale({0.65f, 0.9f, 0.65f});
            Matrix4x4 mvp = proj * view * model;

            glUniformMatrix4fv(u_MVP, 1, GL_FALSE, mvp.Data());
            glUniformMatrix4fv(u_Model, 1, GL_FALSE, model.Data());

            DrawMesh(shardVertices);
        }
    }

    void RenderParticles(const Matrix4x4& proj, const Matrix4x4& view) {
        if (particles.empty()) return;

        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive Bloom Glow

        glUseProgram(particleProgram);
        Matrix4x4 vp = proj * view;
        glUniformMatrix4fv(u_PartVP, 1, GL_FALSE, vp.Data());

        std::vector<float> positions;
        std::vector<float> colors;
        std::vector<float> sizes;

        for (const auto& p : particles) {
            positions.push_back(p.pos.x);
            positions.push_back(p.pos.y);
            positions.push_back(p.pos.z);

            colors.push_back(p.color.x);
            colors.push_back(p.color.y);
            colors.push_back(p.color.z);
            colors.push_back(p.color.w);

            sizes.push_back(p.size);
        }

        glEnableVertexAttribArray(a_PartPos);
        glVertexAttribPointer(a_PartPos, 3, GL_FLOAT, GL_FALSE, 0, positions.data());

        glEnableVertexAttribArray(a_PartColor);
        glVertexAttribPointer(a_PartColor, 4, GL_FLOAT, GL_FALSE, 0, colors.data());

        glEnableVertexAttribArray(a_PartSize);
        glVertexAttribPointer(a_PartSize, 1, GL_FLOAT, GL_FALSE, 0, sizes.data());

        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particles.size()));

        glDisableVertexAttribArray(a_PartPos);
        glDisableVertexAttribArray(a_PartColor);
        glDisableVertexAttribArray(a_PartSize);

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }

    void DrawMesh(const std::vector<Vertex>& vertices) {
        glEnableVertexAttribArray(a_Position);
        glVertexAttribPointer(a_Position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &vertices[0].x);

        glEnableVertexAttribArray(a_Normal);
        glVertexAttribPointer(a_Normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &vertices[0].nx);

        glEnableVertexAttribArray(a_Color);
        glVertexAttribPointer(a_Color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), &vertices[0].r);

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

        glDisableVertexAttribArray(a_Position);
        glDisableVertexAttribArray(a_Normal);
        glDisableVertexAttribArray(a_Color);
    }

    void DrawMeshWithColor(const std::vector<Vertex>& vertices, const Vector4& col) {
        std::vector<Vertex> colored = vertices;
        for (auto& v : colored) {
            v.r = col.x; v.g = col.y; v.b = col.z; v.a = col.w;
        }
        DrawMesh(colored);
    }

    // Geometry Generation Helpers
    void BuildTunnelMesh() {
        tunnelVertices.clear();
        constexpr int SIDES = 6;
        constexpr float RADIUS = 5.2f;
        constexpr float LENGTH = 20.0f;

        for (int i = 0; i < SIDES; ++i) {
            float a0 = (i * 2.0f * PI) / SIDES;
            float a1 = ((i + 1) * 2.0f * PI) / SIDES;

            float x0 = std::cos(a0) * RADIUS;
            float y0 = std::sin(a0) * RADIUS;
            float x1 = std::cos(a1) * RADIUS;
            float y1 = std::sin(a1) * RADIUS;

            // Inward-pointing normals
            float nx = -(x0 + x1) * 0.5f;
            float ny = -(y0 + y1) * 0.5f;
            float len = std::sqrt(nx * nx + ny * ny);
            nx /= len; ny /= len;

            Vector4 c1 = (i % 2 == 0) ? Vector4(0.1f, 0.02f, 0.25f, 1.0f) : Vector4(0.03f, 0.01f, 0.15f, 1.0f);
            Vector4 c2 = Vector4(0.0f, 0.8f, 1.0f, 1.0f); // Neon Cyan Edge lines

            // Triangle 1
            tunnelVertices.push_back({x0, y0, 0.0f, nx, ny, 0.0f, c1.x, c1.y, c1.z, 1.0f});
            tunnelVertices.push_back({x1, y1, 0.0f, nx, ny, 0.0f, c2.x, c2.y, c2.z, 1.0f});
            tunnelVertices.push_back({x1, y1, LENGTH, nx, ny, 0.0f, c1.x, c1.y, c1.z, 1.0f});

            // Triangle 2
            tunnelVertices.push_back({x0, y0, 0.0f, nx, ny, 0.0f, c1.x, c1.y, c1.z, 1.0f});
            tunnelVertices.push_back({x1, y1, LENGTH, nx, ny, 0.0f, c1.x, c1.y, c1.z, 1.0f});
            tunnelVertices.push_back({x0, y0, LENGTH, nx, ny, 0.0f, c2.x, c2.y, c2.z, 1.0f});
        }
    }

    void BuildShipMesh() {
        shipVertices.clear();
        // Sleek 3D Delta Dart Spacecraft
        Vector3 nose = {0.0f, 0.05f, 1.4f};
        Vector3 leftWing = {-1.3f, 0.0f, -0.9f};
        Vector3 rightWing = {1.3f, 0.0f, -0.9f};
        Vector3 cockpitTop = {0.0f, 0.45f, -0.1f};
        Vector3 tailCenter = {0.0f, 0.0f, -0.85f};
        Vector3 belly = {0.0f, -0.2f, -0.2f};

        Vector4 hullColor = {0.1f, 0.15f, 0.25f, 1.0f};      // Deep Metallic Navy
        Vector4 neonWing = {0.0f, 0.95f, 1.0f, 1.0f};        // Glowing Cyan
        Vector4 canopyColor = {1.0f, 0.05f, 0.7f, 1.0f};     // Neon Magenta Cockpit

        auto AddTri = [this](const Vector3& p1, const Vector3& p2, const Vector3& p3,
                             const Vector4& c1, const Vector4& c2, const Vector4& c3) {
            Vector3 norm = (p2 - p1).Cross(p3 - p1).Normalized();
            shipVertices.push_back({p1.x, p1.y, p1.z, norm.x, norm.y, norm.z, c1.x, c1.y, c1.z, c1.w});
            shipVertices.push_back({p2.x, p2.y, p2.z, norm.x, norm.y, norm.z, c2.x, c2.y, c2.z, c2.w});
            shipVertices.push_back({p3.x, p3.y, p3.z, norm.x, norm.y, norm.z, c3.x, c3.y, c3.z, c3.w});
        };

        // Top Hull Left & Right
        AddTri(nose, cockpitTop, leftWing, hullColor, canopyColor, neonWing);
        AddTri(nose, rightWing, cockpitTop, hullColor, neonWing, canopyColor);

        // Cockpit rear
        AddTri(cockpitTop, tailCenter, leftWing, canopyColor, hullColor, neonWing);
        AddTri(cockpitTop, rightWing, tailCenter, canopyColor, neonWing, hullColor);

        // Underbelly
        AddTri(nose, leftWing, belly, hullColor, neonWing, hullColor);
        AddTri(nose, belly, rightWing, hullColor, hullColor, neonWing);
        AddTri(belly, leftWing, tailCenter, hullColor, neonWing, hullColor);
        AddTri(belly, tailCenter, rightWing, hullColor, hullColor, neonWing);
    }

    void BuildCubeMesh() {
        cubeVertices.clear();
        // 3D Unit Cube centered at origin [-0.5, 0.5]
        static const float p[8][3] = {
            {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}
        };

        static const int faces[6][4] = {
            {0, 1, 2, 3}, // Front
            {5, 4, 7, 6}, // Back
            {4, 0, 3, 7}, // Left
            {1, 5, 6, 2}, // Right
            {3, 2, 6, 7}, // Top
            {4, 5, 1, 0}  // Bottom
        };

        static const float normals[6][3] = {
            { 0,  0, -1}, { 0,  0,  1}, {-1,  0,  0},
            { 1,  0,  0}, { 0,  1,  0}, { 0, -1,  0}
        };

        for (int f = 0; f < 6; ++f) {
            float nx = normals[f][0], ny = normals[f][1], nz = normals[f][2];
            int i0 = faces[f][0], i1 = faces[f][1], i2 = faces[f][2], i3 = faces[f][3];

            // Triangle 1
            cubeVertices.push_back({p[i0][0], p[i0][1], p[i0][2], nx, ny, nz, 1.0f, 1.0f, 1.0f, 1.0f});
            cubeVertices.push_back({p[i1][0], p[i1][1], p[i1][2], nx, ny, nz, 1.0f, 1.0f, 1.0f, 1.0f});
            cubeVertices.push_back({p[i2][0], p[i2][1], p[i2][2], nx, ny, nz, 1.0f, 1.0f, 1.0f, 1.0f});

            // Triangle 2
            cubeVertices.push_back({p[i0][0], p[i0][1], p[i0][2], nx, ny, nz, 1.0f, 1.0f, 1.0f, 1.0f});
            cubeVertices.push_back({p[i2][0], p[i2][1], p[i2][2], nx, ny, nz, 1.0f, 1.0f, 1.0f, 1.0f});
            cubeVertices.push_back({p[i3][0], p[i3][1], p[i3][2], nx, ny, nz, 1.0f, 1.0f, 1.0f, 1.0f});
        }
    }

    void BuildShardMesh() {
        shardVertices.clear();
        // 3D Octahedron / Chrono Diamond
        Vector3 top = {0.0f, 1.0f, 0.0f};
        Vector3 bottom = {0.0f, -1.0f, 0.0f};
        Vector3 mid[4] = {
            { 0.7f, 0.0f,  0.0f},
            { 0.0f, 0.0f,  0.7f},
            {-0.7f, 0.0f,  0.0f},
            { 0.0f, 0.0f, -0.7f}
        };

        Vector4 gold = {1.0f, 0.9f, 0.15f, 1.0f};
        Vector4 white = {1.0f, 1.0f, 0.8f, 1.0f};

        auto AddShardTri = [this](const Vector3& p1, const Vector3& p2, const Vector3& p3,
                                  const Vector4& c1, const Vector4& c2, const Vector4& c3) {
            Vector3 norm = (p2 - p1).Cross(p3 - p1).Normalized();
            shardVertices.push_back({p1.x, p1.y, p1.z, norm.x, norm.y, norm.z, c1.x, c1.y, c1.z, c1.w});
            shardVertices.push_back({p2.x, p2.y, p2.z, norm.x, norm.y, norm.z, c2.x, c2.y, c2.z, c2.w});
            shardVertices.push_back({p3.x, p3.y, p3.z, norm.x, norm.y, norm.z, c3.x, c3.y, c3.z, c3.w});
        };

        for (int i = 0; i < 4; ++i) {
            int next = (i + 1) % 4;
            // Top Pyramid
            AddShardTri(top, mid[i], mid[next], white, gold, gold);
            // Bottom Pyramid
            AddShardTri(bottom, mid[next], mid[i], white, gold, gold);
        }
    }
};

} // namespace Game

#endif // GAMEENGINE_HPP
