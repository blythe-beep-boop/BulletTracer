#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include "IPluginInterface.h"

#include <Glacier/ZEntity.h>
#include <Glacier/ZMath.h>
#include <Glacier/ZItem.h>

/**
 * BulletTracer - draws the path of the shots fired in the game. By default only the darts (Sieker,
 * tranquilizer pistol, Serpent's Bite and the like), which are not instant hits but real projectiles that
 * drop; with "Trace all shots" every other shot too.
 *
 * Each shot is a ZHM5Projectile of the game's own. Its set-up method (slot 25 of the vtable at Ghidra
 * 0x141e1f6a0) is redirected here to capture its address and the firing weapon's ammo type.
 * From then on its position is read every frame (+0xf8 of that interface, found by watching its memory in flight) and
 * added to the line until it stops moving. The lines stay until cleared or evicted by the history limits.
 *
 * Darts are identified by ammo type while the weapon fires, never by collision-limited displacement.
 */
class BulletTracer : public IPluginInterface {
public:
    ~BulletTracer() override;

    void Init() override;
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;
    void OnDraw3D(IRenderer* p_Renderer) override;
    void OnDepthDraw3D(IRenderer* p_Renderer) override;

private:
    enum class EKind {
        Unknown,
        Dart,
        Bullet,
    };

    struct NewProjectile {
        void* Projectile;
        EKind Kind;
    };

    struct Trace {
        void* Projectile = nullptr;
        uintptr_t VTable = 0;
        std::vector<SVector3> Points;
        bool Flying = true;
        float StillTime = 0.f; // Game time without moving: while the game is paused it does not pass.
        float Age = 0.f;       // Game time since the shot.
        EKind Kind = EKind::Unknown;
    };

    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    void InstallHook();
    void RemoveHook();
    void UpdateTraces(float p_Delta);
    void TrimTraces();
    void DrawTraces(IRenderer* p_Renderer);
    void SetInfiniteAmmo();
    bool CreateCrippleBox();
    void RemoveCrippleBox();
    void Classify(Trace& p_Trace, EKind p_Kind);

    static void ProjectileSetUp(void* p_This, void* p_Params);

    DECLARE_PLUGIN_DETOUR(BulletTracer, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);

    DECLARE_PLUGIN_DETOUR(BulletTracer, bool, OnFireProjectiles, ZHM5ItemWeapon* th, bool bMayStartSound);

    // Settings.
    bool m_ShowWindow = false;
    bool m_Enabled = true;
    bool m_AlwaysVisible = false;
    bool m_UseDepth = false;
    bool m_AllShots = false;
    bool m_InfiniteAmmo = false;
    float m_Color[4] = { 1.f, 0.35f, 0.1f, 1.f };

    // The hook.
    uintptr_t m_ModuleDelta = 0;
    void** m_Slot = nullptr;
    bool m_HookOk = false;

    // Shots.
    std::mutex m_Mutex;
    std::vector<NewProjectile> m_NewProjectiles; // Filled by the hook, emptied on the frame update.
    std::vector<Trace> m_Traces;
    int m_DartsSeen = 0;
    int m_BulletsSeen = 0;

    // Infinite ammo, as the Player mod does it.
    ZEntityRef m_CrippleBoxEntity;
    bool m_HitmanInfiniteAmmo = false; // What the game says now, read on each frame update.
};

DECLARE_ZHM_PLUGIN(BulletTracer)
