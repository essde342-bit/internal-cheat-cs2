#include "config.h"

namespace Config {
    bool espEnabled = true;
    bool espBox = true;
    bool espSkeleton = true;
    bool espHealth = true;
    bool espName = true;
    bool espTeamCheck = true;
    
    Color colorEnemy = Color(1.f, 0.f, 0.f, 1.f);
    Color colorTeam = Color(0.f, 1.f, 0.f, 1.f);
    Color colorSkeleton = Color(1.f, 1.f, 1.f, 0.8f);
    
    void init() {}
}
