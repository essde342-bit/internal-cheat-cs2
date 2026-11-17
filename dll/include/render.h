#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <cstdint>
#include "vmath.h"

namespace Render {
    bool init(IDXGISwapChain* swapChain);
    void shutdown();
    
    void begin();
    void end();
    
    void draw_line(float x1, float y1, float x2, float y2, uint32_t color);
    void draw_rect(float x, float y, float w, float h, uint32_t color);
    void draw_rect_filled(float x, float y, float w, float h, uint32_t color);
    
    void draw_text(float x, float y, uint32_t color, bool centered, const char* fmt, ...);
    Vec2 get_text_size(const char* text);
    
    uint32_t rgba_to_uint32(float r, float g, float b, float a);
}
