#pragma once

#include <algorithm>
#include <cmath>

#include "framebuffer.h"

struct Vertex2D {
    float x;
    float y;
};

inline float edgeFunction(
    const Vertex2D& a,
    const Vertex2D& b,
    const Vertex2D& p
) {
    return
        (p.x - a.x) * (b.y - a.y)
        -
        (p.y - a.y) * (b.x - a.x);
}

inline void triangle(
    const Vertex2D& v0,
    const Vertex2D& v1,
    const Vertex2D& v2
) {
    int minX = static_cast<int>(
        std::floor(
            std::min({
                v0.x,
                v1.x,
                v2.x
            })
        )
    );

    int maxX = static_cast<int>(
        std::ceil(
            std::max({
                v0.x,
                v1.x,
                v2.x
            })
        )
    );

    int minY = static_cast<int>(
        std::floor(
            std::min({
                v0.y,
                v1.y,
                v2.y
            })
        )
    );

    int maxY = static_cast<int>(
        std::ceil(
            std::max({
                v0.y,
                v1.y,
                v2.y
            })
        )
    );

    minX = std::max(minX, 0);
    minY = std::max(minY, 0);

    maxX = std::min(
        maxX,
        SCREEN_WIDTH - 1
    );

    maxY = std::min(
        maxY,
        SCREEN_HEIGHT - 1
    );

    float area =
        edgeFunction(
            v0,
            v1,
            v2
        );

    if (std::abs(area) < 0.0001f) {
        return;
    }

    for (
        int y = minY;
        y <= maxY;
        y++
    ) {
        for (
            int x = minX;
            x <= maxX;
            x++
        ) {
            Vertex2D p {
                x + 0.5f,
                y + 0.5f
            };

            float w0 =
                edgeFunction(
                    v1,
                    v2,
                    p
                );

            float w1 =
                edgeFunction(
                    v2,
                    v0,
                    p
                );

            float w2 =
                edgeFunction(
                    v0,
                    v1,
                    p
                );

            bool inside =
                (
                    w0 >= 0 &&
                    w1 >= 0 &&
                    w2 >= 0
                )
                ||
                (
                    w0 <= 0 &&
                    w1 <= 0 &&
                    w2 <= 0
                );

            if (inside) {
                point(x, y);
            }
        }
    }
}
