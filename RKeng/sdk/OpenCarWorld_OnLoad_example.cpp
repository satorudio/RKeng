// OpenCarWorld_OnLoad_example.cpp

#include <sdk/IScenePlugin.h>
#include <sdk/EngineAPI.h>
#include <sdk/JoltBridge.h>
#include <sdk/SceneState.h>
#include <sdk/PhysicsState.h>
#include <sdk/WorldGen.h>
#include <sdk/SceneLoad.h>
#include <sdk/Logger.h>
#include <sdk/CarState.h>
#include <sdk/CarLoad.h>
#include <sdk/CarTick.h>
#include <sdk/CarInputPoll.h>
#include <sdk/CarMesh.h>
#include <sdk/MathTypes.h>

#include <cmath>

#ifdef RK_JOLT_ENABLED
#include <Jolt/RegisterTypes.h>
#endif

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace
{
    class OpenCarWorld final : public RKeng::IScenePlugin
    {
    public:
        void OnLoad(RKeng::SceneState& scene,
                    RKeng::PhysicsState& ph,
                    const RKeng::EngineAPI& api) override
        {
            m_api = api;

            RKeng::Logger::Info("OpenCarWorld: OnLoad step 1");

#ifdef RK_JOLT_ENABLED
            RKeng::InitJoltFromEngine(api);
            JPH::RegisterTypes();
#endif
            RKeng::Logger::Info("OpenCarWorld: OnLoad step 2 - InitJolt done");

            RKeng::Logger::Info("OpenCarWorld: step 3 - WorldGen::Generate");
            RKeng::SceneLoad::Run(scene, ph);

            // OptimizeBroadPhase — обязательно после добавления всех статиков,
            // иначе raycast колёс VehicleConstraint ничего не находит и машина
            // проваливается сквозь пол.
            if (api.engineVersion >= 7 && api.OptimizeBroadPhase)
            {
                api.OptimizeBroadPhase(ph);
                RKeng::Logger::Info("OpenCarWorld: BroadPhase optimized");
            }

            // Спавн машины
            RKeng::RK_VehicleDesc vd;
            vd.spawnX = 0.f; vd.spawnY = 2.f; vd.spawnZ = 0.f;
            // halfW/halfH/halfL берём из CarState параметров
            vd.halfW  = RKeng::CAR_VOXELS_W * RKeng::CAR_VOXEL_SIZE * 0.5f;  // ~0.5
            vd.halfH  = 0.2f;
            vd.halfL  = RKeng::CAR_VOXELS_L * RKeng::CAR_VOXEL_SIZE * 0.5f;  // ~1.0
            vd.mass   = m_car.params.mass;

            vd.suspMinLen  = m_car.params.suspensionMinLen;
            vd.suspMaxLen  = m_car.params.suspensionMaxLen;
            vd.suspFreq    = m_car.params.suspensionFrequency;
            vd.suspDamping = m_car.params.suspensionDamping;

            vd.wheelRadius  = m_car.params.wheelRadius;
            vd.wheelWidth   = m_car.params.wheelWidth;
            vd.maxSteerDeg  = m_car.params.maxSteerAngle;

            vd.maxTorque     = m_car.params.maxTorque;
            vd.maxRPM        = m_car.params.maxRPM;
            vd.engineInertia = m_car.params.engineInertia;

            vd.antiRollFront = m_car.params.antiRollFront;
            vd.antiRollRear  = m_car.params.antiRollRear;

            vd.frontFriction = m_car.params.frontFriction;
            vd.rearFriction  = m_car.params.rearFriction;

            m_vehicleHandle = api.SpawnVehicle(ph, vd);
            if (m_vehicleHandle == RKeng::RK_INVALID_VEHICLE)
            {
                RKeng::Logger::Error("OpenCarWorld: SpawnVehicle FAILED");
            }
            else
            {
                RKeng::Logger::Info("OpenCarWorld: vehicleHandle=" +
                                    std::to_string(m_vehicleHandle));
            }

            // Начальная позиция машины — чтобы меш с первого кадра на месте
            m_car.position    = { vd.spawnX, vd.spawnY, vd.spawnZ };
            m_car.orientation = { 1.f, 0.f, 0.f, 0.f };
            m_car.initialized = true;
            m_car.meshDirty   = true;

            // Камера от третьего лица — движок не добавляет offset персонажа
            scene.thirdPersonCamera = true;

            // ── Солнце ──────────────────────────────────────────────────────
            // Направление ОТ источника: слева-сверху-чуть сзади (нормализуем вручную).
            // glm::normalize недоступен без lvalue, поэтому задаём вручную.
            scene.sunDir       = glm::normalize(glm::vec3( 0.5f, -1.0f,  0.4f ));
            scene.sunColor     = { 1.6f, 1.45f, 1.1f };   // тёплый солнечный
            scene.ambientColor = { 0.15f, 0.18f, 0.28f }; // холодный ambient (небо)

            RKeng::Logger::Info("OpenCarWorld: OnLoad done");
        }

        void OnTick(RKeng::SceneState& scene,
                    RKeng::PhysicsState& ph,
                    float dt) override
        {
            if (m_vehicleHandle == RKeng::RK_INVALID_VEHICLE) return;

            // 1. Читаем клавиатуру → car.input (W/S/A/D/Shift)
            RKeng::CarInputPoll::Run(m_car, scene, dt);

            // 2. Передаём инпут в движок (до шага физики)
            RKeng::RK_VehicleInput inp;
            inp.throttle  = m_car.input.throttle;
            inp.brake     = m_car.input.brake;
            inp.steer     = m_car.input.steer;
            inp.handbrake = m_car.input.handbrake ? 1.0f : 0.0f;
            m_api.SetVehicleInput(ph, m_vehicleHandle, inp);

            // 3. Читаем трансформ кузова из физики → обновляем car.position/orientation
            {
                float px, py, pz, qx, qy, qz, qw, vx, vy, vz;
                if (m_api.GetVehicleTransform(ph, m_vehicleHandle,
                                               px, py, pz,
                                               qx, qy, qz, qw,
                                               vx, vy, vz))
                {
                    m_car.position    = { px, py, pz };
                    m_car.orientation = { qw, qx, qy, qz }; // glm: w,x,y,z
                    m_car.velocity    = { vx, vy, vz };

                    float speed = glm::length(m_car.velocity);
                    m_car.speedKph = speed * 3.6f;
                    m_car.meshDirty = true;
                }
            }

            // 4. Пересобираем CPU-меш вокселей если грязный
            if (m_car.meshDirty)
            {
                RKeng::CarMesh::Rebuild(m_car);
                scene.sceneMesh.vertices = m_car.meshVertices;
                scene.sceneMesh.indices  = m_car.meshIndices;
                scene.sceneMesh.dirty    = true;
                m_car.meshDirty = false;
            }

            // 5. Обновляем modelMatrix — трансформ от позиции/ориентации кузова.
            //    Вершины в CarMesh генерируются в local space (вокруг нуля),
            //    поэтому сюда идёт полный world-трансформ.
            {
                RKeng::Mat4 T = glm::translate(RKeng::Mat4(1.f), m_car.position);
                RKeng::Mat4 R = glm::toMat4(m_car.orientation);
                scene.sceneMesh.modelMatrix = T * R;
            }

            // 6. Камера следует за машиной (от третьего лица)
            {
                // camLocalOffset задан в CarState: чуть выше и сзади (в локальных координатах машины)
                glm::vec4 localOff = glm::vec4(m_car.camLocalOffset, 0.f);  // 0 = direction (не точка)
                glm::mat4 R = glm::toMat4(m_car.orientation);
                glm::vec3 worldOff = glm::vec3(R * localOff);

                // Позиция камеры = позиция машины + rotated offset
                glm::vec3 camPos = m_car.position + worldOff;
                scene.player.worldPos.world = glm::dvec3(camPos);

                // Камера смотрит на центр машины (чуть выше основания)
                glm::vec3 carCenter = m_car.position + glm::vec3(0.f, CAR_VOXELS_H * CAR_VOXEL_SIZE * 0.5f, 0.f);
                glm::vec3 toCar = glm::normalize(carCenter - camPos);

                // Извлекаем yaw/pitch из направления взгляда
                float yawToTarget   = std::atan2(toCar.x, toCar.z);
                float pitchToTarget = std::asin(glm::clamp(toCar.y, -1.f, 1.f));

                // Применяем мышиное смещение сверху (camYaw/camPitch обновляются в CarInputPoll)
                scene.input.yaw   = yawToTarget   + m_car.camYaw;
                scene.input.pitch = pitchToTarget + m_car.camPitch;
            }
        }

        void OnUnload(RKeng::SceneState& scene,
                      RKeng::PhysicsState& ph) override
        {
            if (m_vehicleHandle != RKeng::RK_INVALID_VEHICLE)
            {
                m_api.DestroyVehicle(ph, m_vehicleHandle);
                m_vehicleHandle = RKeng::RK_INVALID_VEHICLE;
            }
            RKeng::WorldGen::Destroy(scene, ph);
        }

        const char* GetName() const override { return "OpenCarWorld"; }

    private:
        RKeng::EngineAPI       m_api;
        RKeng::CarState        m_car;
        RKeng::RK_VehicleHandle m_vehicleHandle = RKeng::RK_INVALID_VEHICLE;
    };
}

extern "C"
{
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()  { return new OpenCarWorld(); }
    RK_EXPORT void RK_DestroyScene(RKeng::IScenePlugin* p) { delete p; }
}
