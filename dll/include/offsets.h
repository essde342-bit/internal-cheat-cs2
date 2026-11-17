#pragma once
#include <cstdint>

// Актуальные оффсеты на Ноябрь 2026 (a2x / sezzyaep dumps)
namespace client_dll {
    constexpr uintptr_t dwEntityList = 0x2709010;
    constexpr uintptr_t dwLocalPlayerController = 0x252B0C8;
    constexpr uintptr_t dwLocalPlayerPawn = 0x2553D38;
    constexpr uintptr_t dwViewMatrix = 0x2559640;
}

namespace C_BaseEntity {
    constexpr uintptr_t m_iHealth = 0x34C;
    constexpr uintptr_t m_iTeamNum = 0x3E7;
    constexpr uintptr_t m_lifeState = 0x354;
    constexpr uintptr_t m_pGameSceneNode = 0x330;
}

namespace CGameSceneNode {
    constexpr uintptr_t m_vecOrigin = 0x80;
    constexpr uintptr_t m_modelState = 0x140; // CSkeletonInstance -> CModelState
}

namespace CModelState {
    constexpr uintptr_t m_pBoneArray = 0x80;
}

namespace CBasePlayerController {
    constexpr uintptr_t m_hPawn = 0x6BC;
    constexpr uintptr_t m_iszPlayerName = 0x6FC;
}
