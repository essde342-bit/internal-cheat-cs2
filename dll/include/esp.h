#pragma once
#include <cstdint>
#include "vmath.h"

class CEntityIdentity {
public:
    uintptr_t m_pInstance;
    uintptr_t m_pNext;
    uintptr_t m_pPrev;
};

class C_BaseEntity {
public:
    uintptr_t address;
    
    int get_health();
    int get_team();
    bool is_alive();
    Vec3 get_origin();
    uintptr_t get_game_scene_node();
};

class C_BasePlayerController : public C_BaseEntity {
public:
    const char* get_name();
    uint32_t get_pawn_handle();
};

class CCSPlayerPawn : public C_BaseEntity {
public:
    struct BoneMatrix {
        float matrix[3][4];
    };
    
    uintptr_t get_bone_array();
    int get_bone_count();
};

namespace ESP {
    void render();
}
