// OpenCarWorld.cpp — DLL-сцена: кубы + 500 прыгающих NPC + ходьба игрока.

#include "IScenePlugin.h"
#include "EngineAPI.h"
#include "JoltBridge.h"
#include "SceneState.h"
#include "PhysicsState.h"

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  LCG RNG
// ─────────────────────────────────────────────────────────────────────────────
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed = 42u) : s(seed) {}
    float nextf(float lo, float hi) {
        s = s * 1664525u + 1013904223u;
        return lo + (hi-lo) * ((s & 0xFFFF) / 65535.f);
    }
    uint32_t nextu() {
        s = s * 1664525u + 1013904223u;
        return s;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Куб (падающий)
// ─────────────────────────────────────────────────────────────────────────────
struct Cube {
    glm::vec3  half  { 0.5f };
    glm::vec3  color { 1.f, 0.4f, 0.1f };
    uint32_t   rawID = UINT32_MAX;
    float      lastPx=0,lastPy=0,lastPz=0;
    float      lastQx=0,lastQy=0,lastQz=0,lastQw=1;
};

// ─────────────────────────────────────────────────────────────────────────────
//  NPC
// ─────────────────────────────────────────────────────────────────────────────
struct Npc {
    uint32_t  bodyID  = UINT32_MAX; // Jolt body — только тело (капсула из box)
    glm::vec3 color   { 0.2f, 0.6f, 1.0f };

    // AI state
    float  aiTimer    = 0.f;   // время до следующего решения
    float  aiDirX     = 0.f;   // текущее направление движения (нормализованное XZ)
    float  aiDirZ     = 1.f;
    bool   wantsJump  = false;
    bool   onGround   = false;

    // Кэш трансформа для рендера
    float  px=0,py=0,pz=0;
    float  qx=0,qy=0,qz=0,qw=1;
    bool   transformValid = false;

    // Body half-extents
    static constexpr float BODY_HX = 0.22f;
    static constexpr float BODY_HY = 0.55f; // высота тела (полурост)
    static constexpr float BODY_HZ = 0.22f;
    static constexpr float HEAD_R  = 0.20f; // полуразмер головы
};

// ─────────────────────────────────────────────────────────────────────────────
//  Звук
// ─────────────────────────────────────────────────────────────────────────────
static void PlaySpawnSound(uint32_t batchNum)
{
#ifdef _WIN32
    static const UINT SOUNDS[] = { MB_ICONERROR, MB_ICONWARNING, MB_ICONQUESTION, MB_OK };
    MessageBeep(SOUNDS[batchNum % 4]);
#else
    (void)batchNum;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
//  HUD
// ─────────────────────────────────────────────────────────────────────────────
static void UpdateWindowTitle(float fps, float ramMB,
                               int cubes, int npcs, uint32_t batch, float nextIn)
{
#ifdef _WIN32
    HWND hw = FindWindowA("GLFW30", nullptr);
    if (!hw) {
        DWORD pid = GetCurrentProcessId();
        struct C { DWORD pid; HWND hw; } ctx{pid,nullptr};
        EnumWindows([](HWND h,LPARAM lp)->BOOL{
            auto* c=reinterpret_cast<C*>(lp); DWORD p=0;
            GetWindowThreadProcessId(h,&p);
            if(p==c->pid&&IsWindowVisible(h)){c->hw=h;return FALSE;}
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));
        hw = ctx.hw;
    }
    if (!hw) return;
    wchar_t t[256];
    swprintf(t, 256,
        L"RKeng  |  FPS: %.0f  |  RAM: %.0f MB  |  "
        L"Кубов: %d  |  NPC: %d  |  Батч #%u  |  Следующий: %.1f с",
        fps, ramMB, cubes, npcs, batch, nextIn);
    SetWindowTextW(hw, t);
#else
    (void)fps;(void)ramMB;(void)cubes;(void)npcs;(void)batch;(void)nextIn;
#endif
}

static float GetRamMB()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc{}; pmc.cb=sizeof(pmc);
    if(GetProcessMemoryInfo(GetCurrentProcess(),&pmc,sizeof(pmc)))
        return static_cast<float>(pmc.WorkingSetSize)/(1024.f*1024.f);
#endif
    return 0.f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  PlayerMove
// ─────────────────────────────────────────────────────────────────────────────
namespace PlayerMove
{
    static void Run(RKeng::SceneState& scene, RKeng::PhysicsState& ph,
                    const RKeng::EngineAPI* api)
    {
        const float dt = scene.deltaTime;
        auto& player   = scene.player;
        auto& input    = scene.input;

        constexpr float SENS = 0.1f;
        input.yaw   += input.mouseDeltaX * SENS;
        input.pitch -= input.mouseDeltaY * SENS;
        input.pitch  = std::clamp(input.pitch, -89.f, 89.f);

        player.isCrouching = input.crouch;
        player.isRunning   = input.run && !input.crouch;
        float targetH = player.isCrouching ? player.crouchHeight : player.height;
        player.currentHeight += (targetH - player.currentHeight) * 10.f * dt;

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized || !api) return;
        if (!api->SetPlayerVelocity || !api->GetPlayerVelocity) return;

        float yawRad = input.yaw * RKeng::DEG2RAD;
        float fwdX =  std::sin(yawRad), fwdZ = -std::cos(yawRad);
        float rgtX =  std::cos(yawRad), rgtZ =  std::sin(yawRad);

        float wx=0.f, wz=0.f;
        if (input.forward)  { wx+=fwdX; wz+=fwdZ; }
        if (input.backward) { wx-=fwdX; wz-=fwdZ; }
        if (input.right)    { wx+=rgtX; wz+=rgtZ; }
        if (input.left)     { wx-=rgtX; wz-=rgtZ; }

        float len=std::sqrt(wx*wx+wz*wz);
        if (len>1e-4f){wx/=len;wz/=len;}

        float speed = player.isCrouching ? player.crouchSpeed
                    : player.isRunning   ? player.runSpeed
                    :                      player.walkSpeed;
        wx*=speed; wz*=speed;

        float cvx=0,cvy=0,cvz=0;
        api->GetPlayerVelocity(ph,cvx,cvy,cvz);

        float nvy=cvy;
        if (input.jumpPressed && player.onGround)
            nvy=player.jumpImpulse;
        if (!player.onGround){
            float gY=api->GetGravityY?api->GetGravityY(ph):-9.81f;
            nvy+=gY*dt;
        }
        api->SetPlayerVelocity(ph,wx,nvy,wz);
#else
        float yawRad=input.yaw*RKeng::DEG2RAD;
        float sp=player.isRunning?player.runSpeed:player.walkSpeed;
        if(input.forward) {player.worldPos.world.x-=std::sin(yawRad)*sp*dt; player.worldPos.world.z-=std::cos(yawRad)*sp*dt;}
        if(input.backward){player.worldPos.world.x+=std::sin(yawRad)*sp*dt; player.worldPos.world.z+=std::cos(yawRad)*sp*dt;}
        if(input.left)    {player.worldPos.world.x-=std::cos(yawRad)*sp*dt; player.worldPos.world.z+=std::sin(yawRad)*sp*dt;}
        if(input.right)   {player.worldPos.world.x+=std::cos(yawRad)*sp*dt; player.worldPos.world.z-=std::sin(yawRad)*sp*dt;}
        static float vy=0.f;
        bool ground=(player.worldPos.world.y<=0.001);
        if(input.jumpPressed&&ground)vy=player.jumpImpulse;
        if(!ground)vy-=20.f*dt;else vy=0.f;
        player.worldPos.world.y+=vy*dt;
        if(player.worldPos.world.y<0.0)player.worldPos.world.y=0.0;
        (void)ph;(void)api;
#endif
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Основная сцена
// ─────────────────────────────────────────────────────────────────────────────
namespace
{
    class CubeDropScene final : public RKeng::IScenePlugin
    {
    public:
        const char* GetName() const override { return "CubeDropScene"; }

        void OnLoad(RKeng::SceneState& scene,
                    RKeng::PhysicsState& ph,
                    const RKeng::EngineAPI& api) override
        {
            m_api = api;
            if (m_api.LogInfo) m_api.LogInfo("[CubeScene] OnLoad start");

#ifdef RK_JOLT_ENABLED
            RKeng::InitJoltFromEngine(api);
#endif
            // Пол
            if (m_api.SpawnStaticBox) {
                RKeng::RK_BoxBody floor;
                floor.position    = {0.f,-0.1f,0.f};
                floor.halfExtents = {80.f,0.1f,80.f};
                m_api.SpawnStaticBox(ph, floor);
            }
            // 4 стены
            if (m_api.SpawnStaticBox) {
                auto wall=[&](float cx,float cy,float cz,float hx,float hy,float hz){
                    RKeng::RK_BoxBody b;
                    b.position={cx,cy,cz};
                    b.halfExtents={hx,hy,hz};
                    m_api.SpawnStaticBox(ph,b);
                };
                float W=80.f, H=12.f;
                wall( W,H*.5f,0,  0.5f,H,W+1.f);
                wall(-W,H*.5f,0,  0.5f,H,W+1.f);
                wall(0,H*.5f, W,  W+1.f,H,0.5f);
                wall(0,H*.5f,-W,  W+1.f,H,0.5f);
            }

#ifdef RK_JOLT_ENABLED
            if (ph.physicsSystem)
                ph.physicsSystem->OptimizeBroadPhase();
#endif
            // Игрок
            if (m_api.CreateCharacter) {
                RKeng::RK_CharacterDesc cd;
                cd.spawnX=0.f; cd.spawnY=2.f; cd.spawnZ=5.f;
                cd.capsuleHalfHeight=0.9f;
                cd.capsuleRadius=0.35f;
                cd.maxSlopeAngleDeg=45.f;
                m_api.CreateCharacter(ph,cd);
            }
            scene.player.worldPos.world=glm::dvec3(0.0,2.0,5.0);

            // Инициализация
            m_spawnTimer    = CUBE_SPAWN_INTERVAL;
            m_spawnInterval = CUBE_SPAWN_INTERVAL;
            m_rng           = Rng(9999u);
            m_batchNum      = 0;
            m_fpsTimer      = 0.f;
            m_fpsFrames     = 0;
            m_fpsCurrent    = 0.f;
            m_stillFrames   = 0;

            scene.sceneMesh.instanceData.clear();
            scene.sceneMesh.instanceCount=0;
            scene.sceneMesh.instanceDirty=true;

            // Спавним NPC сразу
            SpawnNpcs(ph, NPC_COUNT);

            if (m_api.LogInfo)
                m_api.LogInfo("[CubeScene] Ready! WASD+Mouse=move  Space=jump  Shift=run  Ctrl=crouch");
        }

        void OnTick(RKeng::SceneState& scene,
                    RKeng::PhysicsState& ph,
                    float dt) override
        {
            m_fpsFrames++;
            m_fpsTimer+=dt;
            if(m_fpsTimer>=0.5f){
                m_fpsCurrent=static_cast<float>(m_fpsFrames)/m_fpsTimer;
                m_fpsTimer=0.f; m_fpsFrames=0;
            }

            PlayerMove::Run(scene,ph,&m_api);

            // Спавн кубов
            m_spawnTimer+=dt;
            if(m_spawnTimer>=m_spawnInterval){
                m_spawnTimer=0.f;
                if((int)m_cubes.size()<MAX_CUBES)
                    SpawnCubeBatch(ph);
                m_spawnInterval=std::max(0.5f,m_spawnInterval*0.92f);
            }

            // AI для NPC
            TickNpcAI(ph, dt);

            // Rebuild instance buffer
            RebuildInstancesIfNeeded(scene, ph);

            float nextIn=std::max(0.f,m_spawnInterval-m_spawnTimer);
            UpdateWindowTitle(m_fpsCurrent, GetRamMB(),
                              (int)m_cubes.size(), (int)m_npcs.size(),
                              m_batchNum, nextIn);
        }

        void OnUnload(RKeng::SceneState& scene,
                      RKeng::PhysicsState& ph) override
        {
            if (m_api.DestroyBody && ph.initialized) {
                for (auto& c : m_cubes)
                    if (c.rawID!=UINT32_MAX) m_api.DestroyBody(ph,c.rawID);
                for (auto& n : m_npcs)
                    if (n.bodyID!=UINT32_MAX) m_api.DestroyBody(ph,n.bodyID);
            }
            m_cubes.clear();
            m_npcs.clear();
            scene.sceneMesh.instanceData.clear();
            scene.sceneMesh.instanceCount=0;
            scene.sceneMesh.instanceDirty=true;
        }

    private:
        // ── Константы ────────────────────────────────────────────────────────
        static constexpr int   NPC_COUNT            = 500;
        static constexpr float NPC_SPEED            = 4.5f;   // м/с горизонталь
        static constexpr float NPC_JUMP_VEL         = 8.0f;   // м/с прыжок
        static constexpr float NPC_AI_MIN_INTERVAL  = 0.8f;   // мин время смены AI
        static constexpr float NPC_AI_MAX_INTERVAL  = 2.5f;   // макс время смены AI
        static constexpr float NPC_JUMP_CHANCE      = 0.40f;  // вероятность прыжка при смене AI
        static constexpr float NPC_ARENA_RADIUS     = 70.f;   // разворот у стены
        static constexpr float GROUND_EPS           = 0.12f;  // порог "на земле" по vy

        static constexpr float CUBE_SPAWN_INTERVAL  = 5.0f;
        static constexpr int   CUBE_SPAWN_COUNT     = 10;
        static constexpr int   MAX_CUBES            = 300;
        static constexpr float SPAWN_RADIUS         = 15.f;
        static constexpr float SPAWN_HEIGHT         = 16.f;
        static constexpr float MOVE_EPSILON         = 0.0005f;

        // ── Данные ───────────────────────────────────────────────────────────
        RKeng::EngineAPI   m_api;
        std::vector<Cube>  m_cubes;
        std::vector<Npc>   m_npcs;
        float              m_spawnTimer    = 0.f;
        float              m_spawnInterval = CUBE_SPAWN_INTERVAL;
        uint32_t           m_batchNum      = 0;
        Rng                m_rng;
        float              m_fpsTimer   = 0.f;
        int                m_fpsFrames  = 0;
        float              m_fpsCurrent = 0.f;
        int                m_stillFrames= 0;
        static constexpr int STILL_FRAMES_SKIP = 4;

        static glm::vec3 Hue(float h){
            h-=std::floor(h);
            return {
                glm::clamp(std::abs(h*6.f-3.f)-1.f,0.f,1.f),
                glm::clamp(2.f-std::abs(h*6.f-2.f),0.f,1.f),
                glm::clamp(2.f-std::abs(h*6.f-4.f),0.f,1.f)
            };
        }

        // ── Спавн NPC ────────────────────────────────────────────────────────
        void SpawnNpcs(RKeng::PhysicsState& ph, int count)
        {
#ifndef RK_JOLT_ENABLED
            (void)ph;(void)count;return;
#else
            if (!ph.bodyInterface) return;
            Rng rng(12345u);
            for (int i=0;i<count;i++){
                Npc npc;
                // Случайная позиция по арене
                float angle = rng.nextf(0.f, 6.283f);
                float dist  = rng.nextf(2.f, NPC_ARENA_RADIUS * 0.85f);
                float cx    = std::cos(angle)*dist;
                float cz    = std::sin(angle)*dist;
                float cy    = 1.2f; // спавним чуть над полом

                // Цвет NPC — оттенки синего/голубого/зелёного
                float hue = rng.nextf(0.5f, 0.75f);
                npc.color = Hue(hue);

                // Создаём Jolt body напрямую через bodyInterface
                // (SpawnDynamicBox делает то же самое внутри движка)
                JPH::BoxShapeSettings ss(JPH::Vec3(Npc::BODY_HX, Npc::BODY_HY, Npc::BODY_HZ));
                ss.SetEmbedded();
                auto res = ss.Create();
                if (res.HasError()) continue;

                JPH::BodyCreationSettings bcs(
                    res.Get(),
                    JPH::RVec3(cx, cy, cz),
                    JPH::Quat::sIdentity(),
                    JPH::EMotionType::Dynamic,
                    RKeng::PhysLayers::DYNAMIC);

                // Ограничиваем вращение по X и Z — NPC не заваливается
                bcs.mMassPropertiesOverride.mInertia =
                    JPH::Mat44::sScale(JPH::Vec3(1e10f, 1.0f, 1e10f));
                bcs.mOverrideMassProperties =
                    JPH::EOverrideMassProperties::CalculateMassAndInertia;

                // Без инерции по X/Z через mAngularDamping — упрощённо:
                // просто высокий linearDamping чтоб не скользили
                bcs.mLinearDamping  = 3.0f;   // сильное торможение — NPC "останавливается"
                bcs.mAngularDamping = 99.f;   // не крутится
                bcs.mFriction       = 0.8f;
                bcs.mRestitution    = 0.1f;
                bcs.mGravityFactor  = 2.0f;   // тяжелее падают

                JPH::BodyID id = ph.bodyInterface->CreateAndAddBody(
                    bcs, JPH::EActivation::Activate);
                if (id.IsInvalid()) continue;

                npc.bodyID = id.GetIndexAndSequenceNumber();

                // Начальное направление AI
                npc.aiTimer = rng.nextf(0.f, NPC_AI_MAX_INTERVAL); // разброс таймеров
                float ad = rng.nextf(0.f, 6.283f);
                npc.aiDirX = std::cos(ad);
                npc.aiDirZ = std::sin(ad);

                m_npcs.push_back(npc);
            }
            if (m_api.LogInfo){
                std::string msg="[CubeScene] Spawned "+std::to_string(m_npcs.size())+" NPCs";
                m_api.LogInfo(msg.c_str());
            }
#endif
        }

        // ── AI тик ───────────────────────────────────────────────────────────
        void TickNpcAI(RKeng::PhysicsState& ph, float dt)
        {
#ifndef RK_JOLT_ENABLED
            (void)ph;(void)dt;return;
#else
            if (!ph.bodyInterface || !ph.initialized) return;

            for (auto& npc : m_npcs) {
                if (npc.bodyID == UINT32_MAX) continue;

                JPH::BodyID jid = JPH::BodyID(npc.bodyID);
                if (!ph.bodyInterface->IsAdded(jid)) continue;

                // Текущая скорость
                JPH::Vec3 vel = ph.bodyInterface->GetLinearVelocity(jid);
                float vy = vel.GetY();

                // "На земле" — если vy почти ноль и тело было движущимся вниз
                npc.onGround = (std::abs(vy) < GROUND_EPS);

                // Позиция — нужна для разворота у стены
                JPH::RVec3 pos = ph.bodyInterface->GetPosition(jid);
                float px = pos.GetX(), pz = pos.GetZ();

                // Разворот у границы арены
                bool hitWall = false;
                if (std::abs(px) > NPC_ARENA_RADIUS || std::abs(pz) > NPC_ARENA_RADIUS){
                    // Отражаем направление к центру
                    float len = std::sqrt(px*px+pz*pz);
                    if (len > 0.01f){
                        npc.aiDirX = -px/len * (0.8f + m_rng.nextf(-0.2f,0.2f));
                        npc.aiDirZ = -pz/len * (0.8f + m_rng.nextf(-0.2f,0.2f));
                        // нормализуем
                        float dl = std::sqrt(npc.aiDirX*npc.aiDirX+npc.aiDirZ*npc.aiDirZ);
                        if (dl>0.01f){npc.aiDirX/=dl;npc.aiDirZ/=dl;}
                    }
                    hitWall = true;
                    npc.aiTimer = 0.f; // сразу принять новое решение
                }

                // Таймер AI
                npc.aiTimer -= dt;
                if (npc.aiTimer <= 0.f || hitWall) {
                    npc.aiTimer = NPC_AI_MIN_INTERVAL +
                                  m_rng.nextf(0.f, NPC_AI_MAX_INTERVAL - NPC_AI_MIN_INTERVAL);

                    // Новое направление (случайный угол)
                    if (!hitWall) {
                        float ad = m_rng.nextf(0.f, 6.283f);
                        npc.aiDirX = std::cos(ad);
                        npc.aiDirZ = std::sin(ad);
                    }

                    // Решение о прыжке
                    npc.wantsJump = (m_rng.nextf(0.f,1.f) < NPC_JUMP_CHANCE);
                }

                // Применяем горизонтальную скорость напрямую (kinematic-like)
                float newVx = npc.aiDirX * NPC_SPEED;
                float newVz = npc.aiDirZ * NPC_SPEED;
                float newVy = vy;

                // Прыжок
                if (npc.wantsJump && npc.onGround) {
                    newVy = NPC_JUMP_VEL;
                    npc.wantsJump = false;
                }

                ph.bodyInterface->SetLinearVelocity(jid, JPH::Vec3(newVx, newVy, newVz));

                // Обнуляем угловую скорость — NPC не должен крутиться
                ph.bodyInterface->SetAngularVelocity(jid, JPH::Vec3::sZero());
            }
#endif
        }

        // ── Спавн кубов ──────────────────────────────────────────────────────
        void SpawnCubeBatch(RKeng::PhysicsState& ph)
        {
            if (!m_api.SpawnDynamicBox) return;
            m_batchNum++;
            float batchHue=m_rng.nextf(0.f,1.f);
            PlaySpawnSound(m_batchNum);

            if(m_api.LogInfo){
                std::string msg="[CubeScene] Batch #"+std::to_string(m_batchNum)
                    +"  interval="+std::to_string((int)(m_spawnInterval*1000))+"ms"
                    +"  total="+std::to_string((int)m_cubes.size()+CUBE_SPAWN_COUNT);
                m_api.LogInfo(msg.c_str());
            }

            for(int i=0;i<CUBE_SPAWN_COUNT;i++){
                Cube cube;
                float hs=m_rng.nextf(0.2f,0.7f);
                cube.half={hs,m_rng.nextf(0.2f,0.8f),hs};
                cube.color=Hue(batchHue+m_rng.nextf(-0.06f,0.06f));

                float cx=m_rng.nextf(-SPAWN_RADIUS,SPAWN_RADIUS);
                float cy=SPAWN_HEIGHT+m_rng.nextf(0.f,5.f);
                float cz=m_rng.nextf(-SPAWN_RADIUS,SPAWN_RADIUS);

                RKeng::RK_DynamicBox db;
                db.cx=cx;db.cy=cy;db.cz=cz;
                db.hx=cube.half.x;db.hy=cube.half.y;db.hz=cube.half.z;
                db.mass=15.f+m_rng.nextf(0.f,85.f);
                db.linearDamping=0.04f;
                db.angularDamping=0.07f;
                db.friction=0.55f;

                cube.rawID=m_api.SpawnDynamicBox(ph,db);
                m_cubes.push_back(cube);
            }
            m_stillFrames=0;
        }

        // ── Instance buffer ───────────────────────────────────────────────────
        // Layout per instance: mat4(16) + color(3) + wire(1) = 20 floats
        void RebuildInstancesIfNeeded(RKeng::SceneState& scene, RKeng::PhysicsState& ph)
        {
#ifndef RK_JOLT_ENABLED
            (void)scene;(void)ph;return;
#else
            if (!m_api.GetBodyTransform || !ph.initialized) return;
            if (m_cubes.empty() && m_npcs.empty()) return;

            m_stillFrames++;
            bool anyMoved=false;

            // Обновляем кэш кубов
            for(auto& cube:m_cubes){
                if(cube.rawID==UINT32_MAX)continue;
                float px,py,pz,qx,qy,qz,qw;
                if(!m_api.GetBodyTransform(ph,cube.rawID,px,py,pz,qx,qy,qz,qw))continue;
                if(std::abs(px-cube.lastPx)>MOVE_EPSILON||std::abs(py-cube.lastPy)>MOVE_EPSILON||
                   std::abs(pz-cube.lastPz)>MOVE_EPSILON||std::abs(qx-cube.lastQx)>MOVE_EPSILON||
                   std::abs(qy-cube.lastQy)>MOVE_EPSILON||std::abs(qz-cube.lastQz)>MOVE_EPSILON||
                   std::abs(qw-cube.lastQw)>MOVE_EPSILON)
                {
                    anyMoved=true;
                    cube.lastPx=px;cube.lastPy=py;cube.lastPz=pz;
                    cube.lastQx=qx;cube.lastQy=qy;cube.lastQz=qz;cube.lastQw=qw;
                }
            }

            // Обновляем кэш NPC (они всегда движутся)
            for(auto& npc:m_npcs){
                if(npc.bodyID==UINT32_MAX)continue;
                float px,py,pz,qx,qy,qz,qw;
                if(!m_api.GetBodyTransform(ph,npc.bodyID,px,py,pz,qx,qy,qz,qw))continue;
                npc.px=px;npc.py=py;npc.pz=pz;
                npc.qx=qx;npc.qy=qy;npc.qz=qz;npc.qw=qw;
                npc.transformValid=true;
                anyMoved=true;
            }

            if(!anyMoved && m_stillFrames>STILL_FRAMES_SKIP) return;

            // Инстансов: кубы×2 (solid+wire) + NPC×3 (тело solid, тело wire, голова)
            const size_t maxInst = m_cubes.size()*2 + m_npcs.size()*3;
            scene.sceneMesh.instanceData.resize(maxInst*20);
            float* dst=scene.sceneMesh.instanceData.data();
            uint32_t visible=0;

            // Frustum AABB тест
            auto frustumTest=[&](glm::vec3 center, glm::vec3 half)->bool{
                if(!scene.frustumReady)return true;
                glm::vec3 mn=center-half, mx=center+half;
                for(const auto& p:scene.frustumPlanes){
                    glm::vec3 pv{
                        p.x>=0.f?mx.x:mn.x,
                        p.y>=0.f?mx.y:mn.y,
                        p.z>=0.f?mx.z:mn.z
                    };
                    if(p.x*pv.x+p.y*pv.y+p.z*pv.z+p.w<0.f)return false;
                }
                return true;
            };

            auto pushInst=[&](glm::mat4& m, glm::vec3 col, float wire){
                for(int c=0;c<4;c++)
                    for(int r=0;r<4;r++)
                        *dst++=m[c][r];
                *dst++=col.r;*dst++=col.g;*dst++=col.b;
                *dst++=wire;
            };

            // ── Кубы ─────────────────────────────────────────────────────────
            for(auto& cube:m_cubes){
                if(cube.rawID==UINT32_MAX)continue;
                glm::vec3 pos(cube.lastPx,cube.lastPy,cube.lastPz);
                glm::quat rot(cube.lastQw,cube.lastQx,cube.lastQy,cube.lastQz);

                // Точный AABB из OBB
                {
                    glm::mat3 rm=glm::mat3_cast(rot);
                    glm::vec3 wh=glm::abs(rm[0])*cube.half.x
                                +glm::abs(rm[1])*cube.half.y
                                +glm::abs(rm[2])*cube.half.z;
                    if(!frustumTest(pos,wh+glm::vec3(0.05f)))continue;
                }

                glm::mat4 M=glm::translate(glm::mat4(1.f),pos)
                           *glm::mat4_cast(rot)
                           *glm::scale(glm::mat4(1.f),cube.half*2.f);

                pushInst(M,cube.color,0.f); ++visible;

                glm::mat4 Mw=glm::translate(glm::mat4(1.f),pos)
                            *glm::mat4_cast(rot)
                            *glm::scale(glm::mat4(1.f),(cube.half+glm::vec3(0.012f))*2.f);
                pushInst(Mw,glm::vec3(0.05f,1.f,0.15f),1.f); ++visible;
            }

            // ── NPC ──────────────────────────────────────────────────────────
            for(auto& npc:m_npcs){
                if(!npc.transformValid)continue;

                glm::vec3 bodyPos(npc.px,npc.py,npc.pz);

                // Фрустум-тест по телу + голове вместе
                glm::vec3 npcCenter=bodyPos+glm::vec3(0.f,Npc::HEAD_R,0.f);
                glm::vec3 npcHalf(Npc::BODY_HX+0.05f,
                                  Npc::BODY_HY+Npc::HEAD_R*2.f+0.05f,
                                  Npc::BODY_HZ+0.05f);
                if(!frustumTest(npcCenter,npcHalf))continue;

                // NPC не вращается (angularDamping=99) → используем identity ротацию
                // но смотрит по направлению движения (опционально, пока identity)
                glm::mat4 bodyM=glm::translate(glm::mat4(1.f),bodyPos)
                               *glm::scale(glm::mat4(1.f),
                                           glm::vec3(Npc::BODY_HX,Npc::BODY_HY,Npc::BODY_HZ)*2.f);
                pushInst(bodyM,npc.color,0.f); ++visible;

                // Wireframe тела
                glm::mat4 bodyMw=glm::translate(glm::mat4(1.f),bodyPos)
                                *glm::scale(glm::mat4(1.f),
                                            glm::vec3(Npc::BODY_HX+0.01f,
                                                      Npc::BODY_HY+0.01f,
                                                      Npc::BODY_HZ+0.01f)*2.f);
                pushInst(bodyMw,glm::vec3(1.f,1.f,0.f),1.f); ++visible;

                // Голова — куб над телом
                glm::vec3 headPos=bodyPos+glm::vec3(0.f,Npc::BODY_HY+Npc::HEAD_R+0.02f,0.f);
                glm::mat4 headM=glm::translate(glm::mat4(1.f),headPos)
                               *glm::scale(glm::mat4(1.f),glm::vec3(Npc::HEAD_R*2.f));
                glm::vec3 headColor=npc.color*1.3f;
                headColor=glm::clamp(headColor,glm::vec3(0.f),glm::vec3(1.f));
                pushInst(headM,headColor,0.f); ++visible;
            }

            scene.sceneMesh.instanceCount=visible;
            scene.sceneMesh.instanceDirty=true;
#endif
        }
    };

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
//  DLL export
// ─────────────────────────────────────────────────────────────────────────────
extern "C"
{
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()
    { return new CubeDropScene(); }

    RK_EXPORT void RK_DestroyScene(RKeng::IScenePlugin* p)
    { delete p; }
}

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    if(fdwReason==DLL_PROCESS_ATTACH){
        HANDLE h=GetStdHandle(STD_OUTPUT_HANDLE);
        const char msg[]="[CubeScene] DLL attached\n";
        DWORD w=0;
        WriteConsoleA(h,msg,sizeof(msg)-1,&w,nullptr);
    }
    return TRUE;
}
#endif
