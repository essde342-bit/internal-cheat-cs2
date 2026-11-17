#include "render.h"
#include <d3dcompiler.h>
#include <vector>
#include <cstdarg>
#include <cstdio>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

struct Vertex {
    float x, y, z, w;
    float r, g, b, a;
    float u, v;
};

namespace Render {
    static ID3D11Device* g_device = nullptr;
    static ID3D11DeviceContext* g_context = nullptr;
    static ID3D11RenderTargetView* g_rtv = nullptr;
    
    static ComPtr<ID3D11InputLayout> g_inputLayout;
    static ComPtr<ID3D11VertexShader> g_vertexShader;
    static ComPtr<ID3D11PixelShader> g_pixelShader;
    static ComPtr<ID3D11PixelShader> g_pixelShaderTex;
    static ComPtr<ID3D11Buffer> g_vertexBuffer;
    static ComPtr<ID3D11RasterizerState> g_rasterizerState;
    static ComPtr<ID3D11BlendState> g_blendState;
    static ComPtr<ID3D11Texture2D> g_fontTexture;
    static ComPtr<ID3D11ShaderResourceView> g_fontSRV;
    static ComPtr<ID3D11SamplerState> g_sampler;
    
    static std::vector<Vertex> g_vertices;
    static bool g_initialized = false;
    
    static float g_charWidth[256];
    static float g_charHeight;
    static float g_texCoordX[256];
    static float g_texCoordY[256];
    static float g_texCoordW[256];
    static float g_texCoordH[256];
    static int g_textureWidth = 512;
    static int g_textureHeight = 512;

    static const char* VS_SOURCE = R"(
        struct VS_INPUT {
            float4 pos : POSITION;
            float4 col : COLOR;
            float2 uv  : TEXCOORD;
        };
        struct VS_OUTPUT {
            float4 pos : SV_POSITION;
            float4 col : COLOR;
            float2 uv  : TEXCOORD;
        };
        VS_OUTPUT main(VS_INPUT input) {
            VS_OUTPUT output;
            output.pos = input.pos;
            output.col = input.col;
            output.uv = input.uv;
            return output;
        }
    )";
    
    static const char* PS_SOLID = R"(
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float4 col : COLOR;
            float2 uv  : TEXCOORD;
        };
        float4 main(PS_INPUT input) : SV_Target {
            return input.col;
        }
    )";

    static const char* PS_TEXTURED = R"(
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float4 col : COLOR;
            float2 uv  : TEXCOORD;
        };
        Texture2D tex : register(t0);
        SamplerState samp : register(s0);
        float4 main(PS_INPUT input) : SV_Target {
            float4 t = tex.Sample(samp, input.uv);
            return float4(input.col.rgb, input.col.a * t.a);
        }
    )";

    void create_font_texture() {
        HDC hdc = CreateCompatibleDC(nullptr);
        HFONT hFont = CreateFontA(16, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, 
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                  ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
        SelectObject(hdc, hFont);
        
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = g_textureWidth;
        bmi.bmiHeader.biHeight = -g_textureHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        void* bits = nullptr;
        HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        SelectObject(hdc, hBitmap);
        
        SetBkColor(hdc, RGB(0, 0, 0));
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        
        memset(bits, 0, g_textureWidth * g_textureHeight * 4);
        
        int x = 0, y = 0;
        int maxHeight = 0;
        
        for (int i = 32; i < 256; i++) {
            SIZE size;
            char c = (char)i;
            GetTextExtentPoint32A(hdc, &c, 1, &size);
            
            if (x + size.cx >= g_textureWidth) {
                x = 0;
                y += maxHeight + 2;
                maxHeight = 0;
            }
            
            TextOutA(hdc, x, y, &c, 1);
            
            g_charWidth[i] = (float)size.cx;
            g_charHeight = (float)size.cy;
            
            g_texCoordX[i] = (float)x / g_textureWidth;
            g_texCoordY[i] = (float)y / g_textureHeight;
            g_texCoordW[i] = (float)size.cx / g_textureWidth;
            g_texCoordH[i] = (float)size.cy / g_textureHeight;
            
            if (size.cy > maxHeight) maxHeight = size.cy;
            x += size.cx + 2;
        }
        g_charHeight = 16.f;
        
        uint32_t* pixels = (uint32_t*)bits;
        for (int i = 0; i < g_textureWidth * g_textureHeight; i++) {
            uint8_t b = (pixels[i] >> 0) & 0xFF;
            uint8_t g = (pixels[i] >> 8) & 0xFF;
            uint8_t r = (pixels[i] >> 16) & 0xFF;
            uint8_t a = (r + g + b) / 3;
            pixels[i] = (a << 24) | (255 << 16) | (255 << 8) | 255;
        }
        
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = g_textureWidth;
        texDesc.Height = g_textureHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        
        D3D11_SUBRESOURCE_DATA subData = {};
        subData.pSysMem = bits;
        subData.SysMemPitch = g_textureWidth * 4;
        
        g_device->CreateTexture2D(&texDesc, &subData, &g_fontTexture);
        g_device->CreateShaderResourceView(g_fontTexture.Get(), nullptr, &g_fontSRV);
        
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        g_device->CreateSamplerState(&sampDesc, &g_sampler);
        
        DeleteObject(hBitmap);
        DeleteObject(hFont);
        DeleteDC(hdc);
    }

    bool init(IDXGISwapChain* swapChain) {
        if (g_initialized) return true;
        
        swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_device));
        g_device->GetImmediateContext(&g_context);
        
        ComPtr<ID3DBlob> vsBlob, psSolidBlob, psTexBlob;
        D3DCompile(VS_SOURCE, strlen(VS_SOURCE), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, nullptr);
        D3DCompile(PS_SOLID, strlen(PS_SOLID), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psSolidBlob, nullptr);
        D3DCompile(PS_TEXTURED, strlen(PS_TEXTURED), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psTexBlob, nullptr);
        
        g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_vertexShader);
        g_device->CreatePixelShader(psSolidBlob->GetBufferPointer(), psSolidBlob->GetBufferSize(), nullptr, &g_pixelShader);
        g_device->CreatePixelShader(psTexBlob->GetBufferPointer(), psTexBlob->GetBufferSize(), nullptr, &g_pixelShaderTex);
        
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        g_device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_inputLayout);
        
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = sizeof(Vertex) * 50000;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        g_device->CreateBuffer(&vbDesc, nullptr, &g_vertexBuffer);
        
        D3D11_RASTERIZER_DESC rsDesc = {};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_NONE;
        g_device->CreateRasterizerState(&rsDesc, &g_rasterizerState);
        
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        g_device->CreateBlendState(&blendDesc, &g_blendState);
        
        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv);
        backBuffer->Release();
        
        create_font_texture();
        
        g_initialized = true;
        return true;
    }
    
    void shutdown() {
        if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
        g_initialized = false;
    }
    
    void begin() {
        g_vertices.clear();
    }
    
    void flush_draw(bool textured) {
        if (g_vertices.empty()) return;
        
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->IASetInputLayout(g_inputLayout.Get());
        g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);
        
        D3D11_MAPPED_SUBRESOURCE mapped;
        g_context->Map(g_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, g_vertices.data(), g_vertices.size() * sizeof(Vertex));
        g_context->Unmap(g_vertexBuffer.Get(), 0);
        
        g_context->RSSetState(g_rasterizerState.Get());
        float blendFactor[4] = { 0, 0, 0, 0 };
        g_context->OMSetBlendState(g_blendState.Get(), blendFactor, 0xFFFFFFFF);
        
        g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);
        g_context->PSSetShader(textured ? g_pixelShaderTex.Get() : g_pixelShader.Get(), nullptr, 0);
        
        if (textured) {
            g_context->PSSetShaderResources(0, 1, g_fontSRV.GetAddressOf());
            g_context->PSSetSamplers(0, 1, g_sampler.GetAddressOf());
        }
        
        g_context->Draw(static_cast<UINT>(g_vertices.size()), 0);
        g_vertices.clear();
    }
    
    void end() {
        flush_draw(false);
    }
    
    void add_vert(float x, float y, uint32_t color, float u = 0.f, float v = 0.f) {
        float r = ((color >> 24) & 0xFF) / 255.f;
        float g = ((color >> 16) & 0xFF) / 255.f;
        float b = ((color >> 8) & 0xFF) / 255.f;
        float a = (color & 0xFF) / 255.f;
        g_vertices.push_back({ x, y, 0.f, 1.f, r, g, b, a, u, v });
    }
    
    void draw_line(float x1, float y1, float x2, float y2, uint32_t color) {
        flush_draw(true);
        add_vert(x1, y1, color);
        add_vert(x2, y2, color);
        add_vert(x1 + 1.f, y1 + 1.f, color);
        add_vert(x2, y2, color);
        add_vert(x1 + 1.f, y1 + 1.f, color);
        add_vert(x2 + 1.f, y2 + 1.f, color);
    }
    
    void draw_rect(float x, float y, float w, float h, uint32_t color) {
        draw_line(x, y, x + w, y, color);
        draw_line(x + w, y, x + w, y + h, color);
        draw_line(x + w, y + h, x, y + h, color);
        draw_line(x, y + h, x, y, color);
    }
    
    void draw_rect_filled(float x, float y, float w, float h, uint32_t color) {
        flush_draw(true);
        add_vert(x, y, color);
        add_vert(x + w, y, color);
        add_vert(x, y + h, color);
        add_vert(x + w, y, color);
        add_vert(x + w, y + h, color);
        add_vert(x, y + h, color);
    }
    
    void draw_text(float x, float y, uint32_t color, bool centered, const char* fmt, ...) {
        char buffer[512];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        
        if (centered) {
            Vec2 size = get_text_size(buffer);
            x -= size.x * 0.5f;
        }
        
        flush_draw(false);
        
        float curX = x;
        for (int i = 0; buffer[i] != '\0'; i++) {
            unsigned char c = buffer[i];
            if (c < 32 || c >= 256) { curX += 4.f; continue; }
            
            float w = g_charWidth[c];
            float h = g_charHeight;
            
            float u0 = g_texCoordX[c];
            float v0 = g_texCoordY[c];
            float u1 = u0 + g_texCoordW[c];
            float v1 = v0 + g_texCoordH[c];
            
            add_vert(curX, y, color, u0, v0);
            add_vert(curX + w, y, color, u1, v0);
            add_vert(curX, y + h, color, u0, v1);
            
            add_vert(curX + w, y, color, u1, v0);
            add_vert(curX + w, y + h, color, u1, v1);
            add_vert(curX, y + h, color, u0, v1);
            
            curX += w;
        }
        flush_draw(true);
    }
    
    Vec2 get_text_size(const char* text) {
        float w = 0.f;
        for (int i = 0; text[i] != '\0'; i++) {
            unsigned char c = text[i];
            if (c < 32 || c >= 256) { w += 4.f; continue; }
            w += g_charWidth[c];
        }
        return { w, g_charHeight };
    }
    
    uint32_t rgba_to_uint32(float r, float g, float b, float a) {
        return (static_cast<uint32_t>(r * 255) << 24) |
               (static_cast<uint32_t>(g * 255) << 16) |
               (static_cast<uint32_t>(b * 255) << 8) |
               static_cast<uint32_t>(a * 255);
    }
}
