#pragma once
#include <Windows.h>
#include <iostream>
#include <cmath>
#include <vector>

// korzystamy z helperów ESP (get_bone_position, world_to_screen, ...)
#include "esp.hpp"

using namespace std;

// ============================================================================
 // SILENTAIM CONFIGURATION
 // ============================================================================
namespace silentaim {
    int bone = 0;                       // 0=Head, 1=Neck, 2=Chest, 3=Stomach, 4=Pelvis
    int bind = VK_RBUTTON;              // Klawisz aktywacji
    float fov = 45.0f;                  // FOV w stopniach
    float max_distance = 200.0f;        // Maksymalna odleg³oœæ w metrach
    float angle_threshold = 6.1f;       // Próg k¹ta dla detekcji "silent"
    bool enable_raycast = false;        // Raycast (wy³¹czony domyœlnie)
    bool enable_facing_check = true;    // Sprawdzanie orientacji celu
    float facing_threshold = 45.0f;     // Próg k¹ta patrzenia
    bool rage_mode = false;             // Tryb rage (>20 stopni)
}

// ============================================================================
 // GTA5 MEMORY OFFSETS (1.50+)
 // ============================================================================
namespace Offsets {
    static uintptr_t WorldPTR = 0;
    static uintptr_t BlipPTR = 0;
    static uintptr_t GlobalPTR = 0;

    static int EntityPosition = 0x90;
    static int EntityHealth = 0x280;
    static int EntityMaxHealth = 0x2A0;
    static int EntityArmor = 0x14B8;

    static int PlayerInfo = 0x10C8;
    static int PlayerVehicle = 0xD28;
    static int PlayerInCar = 0xE44;
    static int PlayerRotation = 0x20;

    static int CurrentWeapon = 0x10C8;
    static int WeaponSlot = 0x58;

    static int BoneMatrix = 0x430;
    static int BoneCount = 0x500;
}

// ============================================================================
 // GTA5 BONE IDS
 // ============================================================================
enum BoneIds {
    BONE_HEAD = 0x796E,
    BONE_NECK = 0x9995,
    BONE_SPINE3 = 0x3779,
    BONE_SPINE2 = 0x24818,
    BONE_SPINE1 = 0x3774,
    BONE_PELVIS = 0xE0FD,
    BONE_L_CLAVICLE = 0xFCD9,
    BONE_R_CLAVICLE = 0x29D2,
};

// ============================================================================
 // VECTOR3 CLASS
 // ============================================================================
class Vector3 final
{
public:
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(const float x, const float y, const float z) : x(x), y(y), z(z) {}

    Vector3 operator + (const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
    Vector3 operator - (const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
    Vector3 operator * (const float& rhs) const { return Vector3(x * rhs, y * rhs, z * rhs); }
    Vector3 operator / (const float& rhs) const { return Vector3(x / rhs, y / rhs, z / rhs); }
    bool operator == (const Vector3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }

    Vector3& operator += (const Vector3& rhs) { return *this = *this + rhs; }
    Vector3& operator -= (const Vector3& rhs) { return *this = *this - rhs; }
    Vector3& operator *= (const float& rhs) { return *this = *this * rhs; }
    Vector3& operator /= (const float& rhs) { return *this = *this / rhs; }

    float Length() const { return sqrtf(x * x + y * y + z * z); }
    float LengthSqr() const { return x * x + y * y + z * z; }
    Vector3 Normalize() const {
        float len = Length();
        if (len == 0) return Vector3(0, 0, 0);
        return *this * (1.0f / len);
    }
    float Distance(const Vector3& rhs) const { return (*this - rhs).Length(); }
    float DistanceSqr(const Vector3& rhs) const { return (*this - rhs).LengthSqr(); }
    float Dot(const Vector3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }

    Vector3 Cross(const Vector3& rhs) const {
        return Vector3(
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        );
    }
};

// ============================================================================
 // MEMORY HELPERS
 // ============================================================================
template<typename T>
T read_mem(uintptr_t address) {
    if (!address) return T();
    T value{};
    SIZE_T bytesRead{};
    ReadProcessMemory(GetCurrentProcess(), (LPCVOID)address, &value, sizeof(T), &bytesRead);
    return value;
}

template<typename T>
bool write_mem(uintptr_t address, T value) {
    if (!address) return false;
    SIZE_T bytesWritten{};
    return WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, &value, sizeof(T), &bytesWritten);
}

// ============================================================================
 // TARGET INFO STRUCTURE
 // ============================================================================
struct TargetInfo {
    uintptr_t entity;
    Vector3 position;
    float distance;
    float angle;
    int bone_id;
    bool is_visible;
    bool facing_player;
    float health;

    TargetInfo() : entity(0), distance(FLT_MAX), angle(FLT_MAX),
        bone_id(0), is_visible(false), facing_player(false), health(0) {
    }
};

// ============================================================================
 // ROTATION TO DIRECTION / ANGLES / FACING
 // ============================================================================
Vector3 RotationToDirection(const Vector3& rotation) {
    float z = rotation.z * (3.14159265f / 180.0f);
    float x = rotation.x * (3.14159265f / 180.0f);
    float num = fabsf(cosf(x));
    return Vector3(-sinf(z) * num, cosf(z) * num, sinf(x));
}

float GetAngleToTarget(const Vector3& attackerPos, const Vector3& victimPos, const Vector3& camDirection) {
    Vector3 toVictim = victimPos - attackerPos;
    float distance = toVictim.Length();

    if (distance == 0.0f) return 180.0f;

    toVictim = toVictim.Normalize();
    float dotProduct = camDirection.Dot(toVictim);
    dotProduct = max(-1.0f, min(1.0f, dotProduct));

    float angle = acosf(dotProduct) * (180.0f / 3.14159265f);
    return angle;
}

bool IsTargetFacingPlayer(const Vector3& attackerPos, const Vector3& victimPos, float victimYaw) {
    Vector3 vectorToAttacker = attackerPos - victimPos;
    float len = vectorToAttacker.Length();

    if (len == 0) return false;

    vectorToAttacker = vectorToAttacker.Normalize();
    float yawRad = victimYaw * (3.14159265f / 180.0f);
    Vector3 victimForward(-sinf(yawRad), cosf(yawRad), 0.0f);

    float dot = vectorToAttacker.Dot(victimForward);
    dot = max(-1.0f, min(1.0f, dot));

    float facingAngle = acosf(dot) * (180.0f / 3.14159265f);

    return (facingAngle < silentaim::facing_threshold ||
        facingAngle > (180.0f - silentaim::facing_threshold));
}

// ============================================================================
 // CALCULATE / NORMALIZE / CLAMP
 // ============================================================================
Vector3 CalculateAngles(const Vector3& from, const Vector3& to) {
    Vector3 delta = to - from;
    float distance = delta.Length();

    if (distance == 0) return Vector3(0, 0, 0);

    float pitch = -asinf(delta.z / distance) * (180.0f / 3.14159265f);
    float yaw = atan2f(delta.y, delta.x) * (180.0f / 3.14159265f);

    return Vector3(pitch, yaw, 0);
}

float NormalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

Vector3 ClampAngle(Vector3 angle) {
    angle.x = max(-89.0f, min(89.0f, angle.x));
    angle.y = NormalizeAngle(angle.y);
    angle.z = 0;
    return angle;
}

// ============================================================================
 // CAMERA / BONE HELPERS (u¿ywaj¹ esp.hpp)
 // ============================================================================
Vector3 GetCameraRotation() {
    // placeholder - pozostawiasz implementacjê dostêpu do kamery
    return Vector3(0, 0, 0);
}

// korzystamy z implementacji z esp.hpp (dok³adne pobieranie pozycji koœci)
Vector3 GetBonePosition(uintptr_t entity, int boneId) {
    if (!entity) return Vector3(0, 0, 0);
    // esp.hpp dostarcza funkcjê get_bone_position -> D3DXVECTOR3
    D3DXVECTOR3 b = get_bone_position(entity, boneId);
    return Vector3(b.x, b.y, b.z);
}

std::vector<uintptr_t> GetAllEntities() {
    std::vector<uintptr_t> entities;
    // TODO: implementacja specyficzna dla silnika / FiveM
    return entities;
}

// ============================================================================
 // ANTICHEAT HELPERS
 // ============================================================================
inline float DistancePointToLine(const Vector3& point, const Vector3& lineStart, const Vector3& lineEnd) {
    Vector3 A = point - lineStart;
    Vector3 B = lineEnd - lineStart;
    float dot = A.Dot(B);
    float lenSq = B.LengthSqr();
    if (lenSq == 0) return A.Length();
    float param = dot / lenSq;
    param = max(0.0f, min(1.0f, param));
    Vector3 nearest = lineStart + (B * param);
    return point.Distance(nearest);
}

// ============================================================================
 // MAIN SILENT AIM FUNCTION (u¿ywa dok³adnych pozycji z esp.hpp)
 // ============================================================================
void SilentAim(uintptr_t worldPtr, uintptr_t localPlayer) {
    if (!localPlayer || !worldPtr) return;

    if (!(GetAsyncKeyState(silentaim::bind) & 0x8000)) return;

    Vector3 localPos;
    localPos.x = read_mem<float>(localPlayer + Offsets::EntityPosition);
    localPos.y = read_mem<float>(localPlayer + Offsets::EntityPosition + 0x4);
    localPos.z = read_mem<float>(localPlayer + Offsets::EntityPosition + 0x8);

    Vector3 cameraRot = GetCameraRotation();
    Vector3 camDirection = RotationToDirection(cameraRot);

    int targetBoneId = BONE_HEAD;
    switch (silentaim::bone) {
    case 0: targetBoneId = BONE_HEAD; break;
    case 1: targetBoneId = BONE_NECK; break;
    case 2: targetBoneId = BONE_SPINE3; break;
    case 3: targetBoneId = BONE_SPINE2; break;
    case 4: targetBoneId = BONE_PELVIS; break;
    }

    TargetInfo bestTarget;
    std::vector<uintptr_t> entities = GetAllEntities();

    for (uintptr_t entity : entities) {
        if (!entity || entity == localPlayer) continue;

        Vector3 bonePos = GetBonePosition(entity, targetBoneId);

        float distance = localPos.Distance(bonePos);
        if (distance > silentaim::max_distance) continue;

        float angle = GetAngleToTarget(localPos, bonePos, camDirection);
        if (angle > silentaim::fov) continue;

        float health = read_mem<float>(entity + Offsets::EntityHealth);
        if (health <= 0) continue;

        bool isFacing = true;
        if (silentaim::enable_facing_check) {
            float victimYaw = read_mem<float>(entity + Offsets::PlayerRotation + 0x8);
            isFacing = IsTargetFacingPlayer(localPos, bonePos, victimYaw);

            if (isFacing) {
                if (angle > 5.0f) {
                    continue;
                }
            }
        }

        if (silentaim::enable_raycast) {
            Vector3 lineEnd = localPos + camDirection * 1000.0f;
            float distToLine = DistancePointToLine(bonePos, localPos, lineEnd);
            if (distToLine > 2.0f + (distance * 0.01f)) {
                continue;
            }
        }

        if (angle < bestTarget.angle) {
            bestTarget.entity = entity;
            bestTarget.position = bonePos;
            bestTarget.distance = distance;
            bestTarget.angle = angle;
            bestTarget.bone_id = targetBoneId;
            bestTarget.is_visible = true;
            bestTarget.facing_player = isFacing;
            bestTarget.health = health;
        }
    }

    if (!bestTarget.entity) return;

    if (bestTarget.facing_player) {
        if (bestTarget.angle > 5.0f) return;
    }

    if (bestTarget.angle > 20.0f) return;

    if (bestTarget.angle > silentaim::angle_threshold) return;

    Vector3 aimAngles = CalculateAngles(localPos, bestTarget.position);
    aimAngles = ClampAngle(aimAngles);

    // TODO: apply aimAngles to bullet direction / viewangles
}
 
// ============================================================================
 // SET BIND / INIT
 // ============================================================================
void SetSilentAimBind(int bind_option) {
    switch (bind_option) {
    case 0: silentaim::bind = VK_RBUTTON; break;
    case 1: silentaim::bind = VK_LBUTTON; break;
    case 2: silentaim::bind = VK_XBUTTON1; break;
    case 3: silentaim::bind = VK_XBUTTON2; break;
    case 4: silentaim::bind = VK_SHIFT; break;
    case 5: silentaim::bind = VK_MENU; break;
    case 6: silentaim::bind = 0x45; break;
    default: silentaim::bind = VK_RBUTTON; break;
    }
}

bool InitializeSilentAim() {
    return true;
}