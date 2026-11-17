add_library(cs2_internal SHARED
    dllmain.cpp
    src/globals.cpp
    src/memory.cpp
    src/hooks.cpp
    src/render.cpp
    src/esp.cpp
    src/config.cpp
    src/vmath.cpp
)

target_include_directories(cs2_internal PRIVATE include)
target_link_libraries(cs2_internal PRIVATE d3d11 dxgi dwmapi d3dcompiler)
target_compile_definitions(cs2_internal PRIVATE UNICODE _UNICODE)
