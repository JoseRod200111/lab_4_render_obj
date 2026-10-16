#pragma once

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstring>
#include <vector>

#include "color.h"

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

inline std::vector<Color> framebuffer(
    SCREEN_WIDTH * SCREEN_HEIGHT,
    Color(0, 0, 0, 255)
);

inline Color currentColor(255, 255, 255, 255);

inline void setColor(const Color& color) {
    currentColor = color;
}

inline void clear() {
    std::fill(
        framebuffer.begin(),
        framebuffer.end(),
        Color(20, 22, 30, 255)
    );
}

inline void point(int x, int y) {
    if (
        x < 0 ||
        x >= SCREEN_WIDTH ||
        y < 0 ||
        y >= SCREEN_HEIGHT
    ) {
        return;
    }

    framebuffer[
        y * SCREEN_WIDTH + x
    ] = currentColor;
}

inline void RenderBuffer(SDL_Renderer* renderer) {
    static SDL_Texture* texture = nullptr;

    if (texture == nullptr) {
        texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ABGR8888,
            SDL_TEXTUREACCESS_STREAMING,
            SCREEN_WIDTH,
            SCREEN_HEIGHT
        );
    }

    void* pixels = nullptr;
    int pitch = 0;

    if (
        SDL_LockTexture(
            texture,
            nullptr,
            &pixels,
            &pitch
        ) != 0
    ) {
        return;
    }

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        std::memcpy(
            static_cast<uint8_t*>(pixels)
                + y * pitch,

            framebuffer.data()
                + y * SCREEN_WIDTH,

            SCREEN_WIDTH * sizeof(Color)
        );
    }

    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);

    SDL_RenderCopy(
        renderer,
        texture,
        nullptr,
        nullptr
    );
}
