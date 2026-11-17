#include "esp.h"
#include "globals.h"
#include "offsets.h"
#include "render.h"
#include "config.h"
#include "vmath.h"
#include <cstring>
#include <cmath>

int C_BaseEntity::get_health() {
    return *reinterpret_cast<int*>(address + C_BaseEntity::m_iHealth);
}

int C_BaseEntity::get_team() {
    return *reinterpret_cast<int*>(address + C_BaseEntity::m_iTeamNum);
}

bool C_BaseEntity::is_alive() {
    uint8_t lifeState = *reinterpret_cast<uint8_t*>(address + C_BaseEntity::m_lifeState);
    return lifeState == 0;
}

uintptr_t C_BaseEntity::get_game_scene_node() {
    return *reinterpret_cast<uintptr_t*>(address + C_BaseEntity::m_pGameSceneNode);
}

Vec3 C_BaseEntity::get_origin() {
    uintptr_t node = get_game_scene_node();
    if (!node) return Vec3();
    return *reinterpret_cast<Vec3*>(node + CGameSceneNode::m_vecOrigin);
}

const char* C_BasePlayerController::get_name() {
    return reinterpret_cast<const char*>(address + CBasePlayerController::m_iszPlayerName);
}

uint32_t C_BasePlayerController::get_pawn_handle() {
    return *reinterpret_cast<uint32_t*>(address + CBasePlayerController::m_hPawn);
}

uintptr_t CCSPlayerPawn::get_bone_array() {
    uintptr_t sceneNode = get_game_scene_node();
    if (!sceneNode) return 0;
    
    uintptr_t modelState = *reinterpret_cast<uintptr_t*>(sceneNode + CGameSceneNode::m_modelState);
    if (!modelState) return 0;
    
    return *reinterpret_cast<uintptr_t*>(modelState + CModelState::m_pBoneArray);
}

int CCSPlayerPawn::get_bone_count() {
    return 64;
}

namespace ESP {
    void draw_box(const Vec2& head, const Vec2& feet, uint32_t color) {
        float height = fabsf(feet.y - head.y);
        float width = height * 0.4f;
        float x = head.x - width * 0.5f;
        float y = head.y;
        
        uint32_t black = Render::rgba_to_uint32(0.f, 0.f, 0.f, 1.f);
        Render::draw_rect(x - 1, y - 1, width + 2, height + 2, black);
        Render::draw_rect(x + 1, y + 1, width - 2, height - 2, black);
        Render::draw_rect(x, y, width, height, color);
    }
    
    void draw_health_bar(const Vec2& head, const Vec2& feet, int health) {
        float height = fabsf(feet.y - head.y);
        float width = height * 0.4f;
        float x = head.x - width * 0.5f - 6.f;
        float y = head.y;
        
        float healthPercent = (health > 100) ? 1.f : (health / 100.f);
        float healthHeight = height * healthPercent;
        
        uint32_t bg = Render::rgba_to_uint32(0.f, 0.f, 0.f, 0.7f);
        uint32_t color = Render::rgba_to_uint32(1.f - healthPercent, healthPercent, 0.f, 1.f);
        
        Render::draw_rect_filled(x, y, 4.f, height, bg);
        Render::draw_rect_filled(x, y + height - healthHeight, 4.f, healthHeight, color);
    }
    
    void draw_skeleton(uintptr_t pawnAddr, const ViewMatrix& viewMatrix) {
        CCSPlayerPawn pawn;
        pawn.address = pawnAddr;
        
        uintptr_t boneArray = pawn.get_bone_array();
        if (!boneArray) return;
        
        int boneCount = pawn.get_bone_count();
        if (boneCount <= 0 || boneCount > 128) return;
        
        CCSPlayerPawn::BoneMatrix* bones = reinterpret_cast<CCSPlayerPawn::BoneMatrix*>(boneArray);
        
        struct BoneConnection { int from; int to; };
        BoneConnection connections[] = {
            {0, 5}, {5, 7}, {7, 8}, {8, 9}, {9, 10}, {7, 11}, {11, 12}, {12, 13},
            {5, 14}, {14, 15}, {15, 16}, {5, 17}, {17, 18}, {18, 19}
        };
        
        uint32_t color = Render::rgba_to_uint32(Config::colorSkeleton.r, Config::colorSkeleton.g, 
                                                Config::colorSkeleton.b, Config::colorSkeleton.a);
        
        for (const auto& conn : connections) {
            if (conn.from >= boneCount || conn.to >= boneCount) continue;
            
            Vec3 pos1 = { bones[conn.from].matrix[0][3], bones[conn.from].matrix[1][3], bones[conn.from].matrix[2][3] };
            Vec3 pos2 = { bones[conn.to].matrix[0][3], bones[conn.to].matrix[1][3], bones[conn.to].matrix[2][3] };
            
            Vec2 screen1, screen2;
            if (Math::world_to_screen(pos1, screen1, viewMatrix, Globals::screenWidth, Globals::screenHeight) &&
                Math::world_to_screen(pos2, screen2, viewMatrix, Globals::screenWidth, Globals::screenHeight)) {
                Render::draw_line(screen1.x, screen1.y, screen2.x, screen2.y, color);
            }
        }
    }

    void render() {
        if (!Config::espEnabled || !Globals::entityList || !Globals::localPlayerPawn || !Globals::viewMatrix) return;
        
        ViewMatrix viewMatrix = *reinterpret_cast<ViewMatrix*>(Globals::viewMatrix);
        
        C_BaseEntity localPlayer;
        localPlayer.address = Globals::localPlayerPawn;
        int localTeam = localPlayer.get_team();
        
        uintptr_t entitySystem = *reinterpret_cast<uintptr_t*>(Globals::entityList);
        if (!entitySystem) return;
        
        CEntityIdentity** identityList = reinterpret_cast<CEntityIdentity**>(entitySystem + 0x10);
        
        for (int i = 0; i < 4096; ++i) {
            CEntityIdentity* identity = identityList[i];
            if (!identity || !identity->m_pInstance) continue;
            
            uintptr_t entityAddr = identity->m_pInstance;
            if (entityAddr == Globals::localPlayerPawn) continue;
            
            C_BaseEntity entity;
            entity.address = entityAddr;
            
            int team = entity.get_team();
            if (team != 2 && team != 3) continue;
            if (!entity.is_alive()) continue;
            
            int health = entity.get_health();
            if (health <= 0 || health > 100) continue;
            
            if (Config::espTeamCheck && team == localTeam) continue;
            
            Vec3 origin = entity.get_origin();
            if (origin.is_zero()) continue;
            
            Vec2 screenPos;
            if (!Math::world_to_screen(origin, screenPos, viewMatrix, Globals::screenWidth, Globals::screenHeight))
                continue;
            
            Vec3 headPos = origin + Vec3(0.f, 0.f, 72.f);
            Vec3 feetPos = origin;
            
            Vec2 headScreen, feetScreen;
            if (!Math::world_to_screen(headPos, headScreen, viewMatrix, Globals::screenWidth, Globals::screenHeight))
                continue;
            Math::world_to_screen(feetPos, feetScreen, viewMatrix, Globals::screenWidth, Globals::screenHeight);
            
            uint32_t color = (team == localTeam) ? 
                Render::rgba_to_uint32(Config::colorTeam.r, Config::colorTeam.g, Config::colorTeam.b, Config::colorTeam.a) :
                Render::rgba_to_uint32(Config::colorEnemy.r, Config::colorEnemy.g, Config::colorEnemy.b, Config::colorEnemy.a);
            
            if (Config::espBox) draw_box(headScreen, feetScreen, color);
            if (Config::espHealth) draw_health_bar(headScreen, feetScreen, health);
            
            if (Config::espName) {
                Render::draw_text(headScreen.x, headScreen.y - 15.f, color, true, "HP: %d", health);
            }
            
            if (Config::espSkeleton) draw_skeleton(entityAddr, viewMatrix);
        }
    }
}
