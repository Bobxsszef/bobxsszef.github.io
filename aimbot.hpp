#pragma once
#include <Windows.h>
#include <iostream>
#include <cmath>
#include <vector>

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
    // World PTR patterns - te trzeba znaleŸæ dynamicznie
    static uintptr_t WorldPTR = 0;
    static uintptr_t BlipPTR = 0;
    static uintptr_t GlobalPTR = 0;

    // Entity offsets
    static int EntityPosition = 0x90;           // X,Y,Z (+0x90, +0x94, +0x98)
    static int EntityHealth = 0x280;            // Health
    static int EntityMaxHealth = 0x2A0;         // Max health
    static int EntityArmor = 0x14B8;            // Armor

    // Player offsets
    static int PlayerInfo = 0x10C8;             // PlayerInfo struct
    static int PlayerVehicle = 0xD28;           // Pointer to vehicle
    static int PlayerInCar = 0xE44;             // In vehicle flag
    static int PlayerRotation = 0x20;           // Rotation

    // Weapon offsets
    static int CurrentWeapon = 0x10C8;          // Current weapon ptr
    static int WeaponSlot = 0x58;               // Weapon slot

    // Bone offsets (CPed)
    static int BoneMatrix = 0x430;              // Bone matrix array
    static int BoneCount = 0x500;               // Przybli¿ony offset
}

// ============================================================================
// GTA5 BONE IDS
// ============================================================================
enum BoneIds {
    BONE_HEAD = 0x796E,         // 31086 - G³owa
    BONE_NECK = 0x9995,         // 39317 - Szyja
    BONE_SPINE3 = 0x3779,       // 14201 - Górna klatka
    BONE_SPINE2 = 0x24818,      // 149528 - Œrodkowa klatka
    BONE_SPINE1 = 0x3774,       // 14196 - Dolna klatka
    BONE_PELVIS = 0xE0FD,       // 57597 - Miednica
    BONE_L_CLAVICLE = 0xFCD9,   // 64729 - Lewe ramiê
    BONE_R_CLAVICLE = 0x29D2,   // 10706 - Prawe ramiê
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
    T value;
    SIZE_T bytesRead;
    ReadProcessMemory(GetCurrentProcess(), (LPCVOID)address, &value, sizeof(T), &bytesRead);
    return value;
}

template<typename T>
bool write_mem(uintptr_t address, T value) {
    if (!address) return false;
    SIZE_T bytesWritten;
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
// ROTATION TO DIRECTION (z Lua anticheata)
// ============================================================================
Vector3 RotationToDirection(const Vector3& rotation) {
    float z = rotation.z * (3.14159265f / 180.0f);
    float x = rotation.x * (3.14159265f / 180.0f);
    float num = fabsf(cosf(x));
    return Vector3(-sinf(z) * num, cosf(z) * num, sinf(x));
}

// ============================================================================
// GET ANGLE TO TARGET (z Lua anticheata)
// ============================================================================
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

// ============================================================================
// IS TARGET FACING PLAYER (z Lua anticheata)
// ============================================================================
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
        facingAngle >(180.0f - silentaim::facing_threshold));
}

// ============================================================================
// CALCULATE ANGLES TO TARGET
// ============================================================================
Vector3 CalculateAngles(const Vector3& from, const Vector3& to) {
    Vector3 delta = to - from;
    float distance = delta.Length();

    if (distance == 0) return Vector3(0, 0, 0);

    float pitch = -asinf(delta.z / distance) * (180.0f / 3.14159265f);
    float yaw = atan2f(delta.y, delta.x) * (180.0f / 3.14159265f);

    return Vector3(pitch, yaw, 0);
}

// ============================================================================
// NORMALIZE ANGLE
// ============================================================================
float NormalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

// ============================================================================
// CLAMP ANGLE
// ============================================================================
Vector3 ClampAngle(Vector3 angle) {
    angle.x = max(-89.0f, min(89.0f, angle.x));
    angle.y = NormalizeAngle(angle.y);
    angle.z = 0;
    return angle;
}

// ============================================================================
// GET CAMERA ROTATION
// ============================================================================
Vector3 GetCameraRotation() {
    // TODO: ZnajdŸ prawdziwy offset do kamery
    // Na razie zwracamy placeholder
    // W prawdziwej implementacji u¿yj GetGameplayCamRot()
    return Vector3(0, 0, 0);
}

// ============================================================================
// GET BONE POSITION
// ============================================================================
Vector3 GetBonePosition(uintptr_t entity, int boneId) {
    if (!entity) return Vector3(0, 0, 0);

    // Pobierz base position entity
    Vector3 basePos;
    basePos.x = read_mem<float>(entity + Offsets::EntityPosition);
    basePos.y = read_mem<float>(entity + Offsets::EntityPosition + 0x4);
    basePos.z = read_mem<float>(entity + Offsets::EntityPosition + 0x8);

    // TODO: Implementuj w³aœciwe pobieranie pozycji koœci
    // W GTA5 koœci s¹ w bone matrix, wymagaj¹ transformacji
    // Na razie zwracamy base position jako placeholder

    return basePos;
}

// ============================================================================
// GET ALL ENTITIES
// ============================================================================
std::vector<uintptr_t> GetAllEntities() {
    std::vector<uintptr_t> entities;

    // TODO: Implementuj pobieranie listy wszystkich graczy/pedów
    // Przyk³ad dla FiveM:
    // - Iteruj po ped pool
    // - SprawdŸ czy entity jest valid
    // - Dodaj do listy

    return entities;
}

// ============================================================================
// MAIN SILENT AIM FUNCTION
// ============================================================================
void SilentAim(uintptr_t worldPtr, uintptr_t localPlayer) {
    if (!localPlayer || !worldPtr) return;

    // SprawdŸ czy klawisz jest wciœniêty
    if (!(GetAsyncKeyState(silentaim::bind) & 0x8000)) return;

    // Pobierz pozycjê gracza
    Vector3 localPos;
    localPos.x = read_mem<float>(localPlayer + Offsets::EntityPosition);
    localPos.y = read_mem<float>(localPlayer + Offsets::EntityPosition + 0x4);
    localPos.z = read_mem<float>(localPlayer + Offsets::EntityPosition + 0x8);

    // Pobierz rotacjê kamery
    Vector3 cameraRot = GetCameraRotation();
    Vector3 camDirection = RotationToDirection(cameraRot);

    // Okreœl koœæ do celowania
    int targetBoneId = BONE_HEAD;
    switch (silentaim::bone) {
    case 0: targetBoneId = BONE_HEAD; break;
    case 1: targetBoneId = BONE_NECK; break;
    case 2: targetBoneId = BONE_SPINE3; break;
    case 3: targetBoneId = BONE_SPINE2; break;
    case 4: targetBoneId = BONE_PELVIS; break;
    }

    // ZnajdŸ najlepszy cel
    TargetInfo bestTarget;
    std::vector<uintptr_t> entities = GetAllEntities();

    for (uintptr_t entity : entities) {
        if (!entity || entity == localPlayer) continue;

        // Pobierz pozycjê koœci celu
        Vector3 bonePos = GetBonePosition(entity, targetBoneId);

        // SprawdŸ dystans
        float distance = localPos.Distance(bonePos);
        if (distance > silentaim::max_distance) continue;

        // SprawdŸ k¹t do celu
        float angle = GetAngleToTarget(localPos, bonePos, camDirection);
        if (angle > silentaim::fov) continue;

        // SprawdŸ zdrowie
        float health = read_mem<float>(entity + Offsets::EntityHealth);
        if (health <= 0) continue;

        // SprawdŸ orientacjê (opcjonalne)
        bool isFacing = true;
        if (silentaim::enable_facing_check) {
            float victimYaw = read_mem<float>(entity + Offsets::PlayerRotation + 0x8);
            isFacing = IsTargetFacingPlayer(localPos, bonePos, victimYaw);
        }

        // Wybierz cel z najmniejszym k¹tem
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

    // SprawdŸ typ detekcji (z anticheata)
    if (bestTarget.facing_player) {
        if (bestTarget.angle > 20.0f && !silentaim::rage_mode) {
            return; // Zbyt agresywne dla normalnego trybu
        }
        if (bestTarget.angle > silentaim::angle_threshold && bestTarget.angle <= 20.0f) {
            // Silent aim detection - OK
        }
    }

    // Oblicz k¹ty do celu
    Vector3 aimAngles = CalculateAngles(localPos, bestTarget.position);
    aimAngles = ClampAngle(aimAngles);

    // MODYFIKUJ K¥T STRZA£U
    // TODO: ZnajdŸ offset do view angles lub bullet direction
    // Przyk³ad:
    // write_mem<Vector3>(VIEWANGLES_OFFSET, aimAngles);
}

// ============================================================================
// SET BIND KEY
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

// ============================================================================
// INITIALIZE OFFSETS
// ============================================================================
bool InitializeSilentAim() {
    // TODO: ZnajdŸ dynamiczne offsety u¿ywaj¹c pattern scanning
    // Przyk³ad wzorców z GitHub:
    // WorldPTR: 48 8B 05 ? ? ? ? 45 ? ? ? ? 48 8B 48 08 48 85 C9 74 07
    // BlipPTR: 4C 8D 05 ? ? ? ? 0F B7 C1
    // GlobalPTR: 4C 8D 05 ? ? ? ? 4D 8B 08 4D 85 C9 74 11

    return true;
}