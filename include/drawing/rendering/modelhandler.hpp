#pragma once
#include "drawing/rendering/model.hpp"
#include "config.hpp"


class ModelHandler {
    public:
        ModelHandler(){
            pixelBitmap = nullptr;
        }
        //Used to mark pixels that were drawn and dont need to draw twice
        void Allocate(){
            pixelBitmap = (uint8_t*)heap_caps_aligned_alloc( 32,  CANVAS_HEIGHT * (CANVAS_WIDTH/8), MALLOC_CAP_8BIT);
        }
    
        void RenderScene(std::vector<Model*> mdls, ShaderType shader, float shaderStrenght);
        void RenderModels(std::vector<Model*> mdls);

        int addModel(Model *m){
            models.emplace_back(m);
            return models.size() -1;
        };
                    
        inline bool IRAM_ATTR MarkPixel(int x, int y) {
            uint8_t* bytePtr = &pixelBitmap[(y * (CANVAS_WIDTH/8)) + (x >> 3)];
            uint8_t mask = 1 << (x & 7);
            if (*bytePtr & mask) return true;
            *bytePtr |= mask;
            return false;
        }

        uint8_t *pixelBitmap; 
        std::vector<Model*> models;
};
