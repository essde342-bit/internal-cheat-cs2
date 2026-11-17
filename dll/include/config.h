#pragma once
#include <cstdint>

struct Color {
    float r, g, b, a;
    Color() : r(1), g(1), b(1), a(1) {}
    Color(float r, float g, float b, float a = 1.f) : r(r), g(g), b(b), a(a) {}
};

namespace Config {
    extern bool espEnabled;
    extern bool espBox;
    extern bool espSkeleton;
    extern bool espHealth;
    extern bool espName;
    extern bool espTeamCheck;
    
    extern Color colorEnemy;
    extern Color colorTeam;
    extern Color colorSkeleton;
    
    void init();
}
