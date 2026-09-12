#include "drawing/framebuffer.hpp"
#include "drawing/rendering/shader.hpp"
#include "tools/config_default.hpp"
#include "tools/hardwareconfig.hpp"
#include "tools/devices.hpp"


bool FrameBuffer::Allocate(){
    if (pixels != nullptr){
        return false;
    }
    pixels = (uint16_t*)ps_malloc(sizeof(uint16_t) * CANVAS_HEIGHT * CANVAS_WIDTH);
    sizeX = CANVAS_WIDTH;
    sizeY = CANVAS_HEIGHT;
    return pixels != nullptr;
}    

void FrameBuffer::ClearFrameBuffer(){
    uint8_t r, g, b;
    int16_t y,x;
    int pixelId = 0;
    for (y = 0; y < sizeY; y++) {
        pixelId = y * sizeX;
        for (x = 0; x < sizeX; x++) {
            pixels[pixelId + x] = 0;
        }
    }
}

void FrameBuffer::DrawFrameBuffer(FlipConfig &flipSettings, ShaderType shader, float shaderStrenght){
    int16_t y,x;
    uint16_t color;
    int pixelId = 0;
    uint8_t r,g,b;
    for (y = 0; y < sizeY; y++) {
        pixelId = y * sizeX;
        for (x = 0; x < sizeX; x++) {
            color = pixels[pixelId + x];
            Devices::Display->color565to888(color, r, g, b);
            ShaderProcessor::UpdateColorByShader(x, y, r, g, b, shader, shaderStrenght, this);
            Devices::Display->setPixelWithFlip(x, y, r, g, b, flipSettings);
        }
    }
}