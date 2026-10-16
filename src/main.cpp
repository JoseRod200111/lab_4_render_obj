#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "color.h"
#include "framebuffer.h"
#include "triangle.h"

// ============================================================
// ESTRUCTURAS
// ============================================================

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Face {
    int a;
    int b;
    int c;
};

struct TriangleToDraw {
    Vertex2D a;
    Vertex2D b;
    Vertex2D c;

    float depth;

    Color color;
};

// ============================================================
// SDL
// ============================================================

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// ============================================================
// MODELO
// ============================================================

std::vector<Vec3> vertices;
std::vector<Face> faces;

// ============================================================
// INICIALIZACION
// ============================================================

void init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout
            << "Error inicializando SDL: "
            << SDL_GetError()
            << std::endl;

        return;
    }

    window = SDL_CreateWindow(
        "Lab 4 - Render OBJ",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr) {
        std::cout
            << "Error creando ventana: "
            << SDL_GetError()
            << std::endl;

        return;
    }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (renderer == nullptr) {
        std::cout
            << "Error creando renderer: "
            << SDL_GetError()
            << std::endl;
    }
}

// ============================================================
// LECTURA DEL OBJ
// ============================================================

// Lee el numero de vertice de expresiones OBJ como:
//
// 1
// 1/2
// 1/2/3
// 1//3
//
// OBJ empieza sus indices en 1.
// En C++ los vectores empiezan en 0.
int readIndex(
    const std::string& token,
    int vertexCount
) {
    std::string number =
        token.substr(
            0,
            token.find('/')
        );

    int index =
        std::stoi(number);

    if (index > 0) {
        return index - 1;
    }

    // OBJ tambien permite indices negativos.
    return vertexCount + index;
}

bool loadOBJ(
    const std::string& filename
) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout
            << "No se pudo abrir el archivo OBJ: "
            << filename
            << std::endl;

        return false;
    }

    vertices.clear();
    faces.clear();

    std::string line;

    while (
        std::getline(
            file,
            line
        )
    ) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);

        std::string type;

        ss >> type;

        // ----------------------------------------------------
        // VERTICE
        //
        // Ejemplo:
        // v 1.0 2.0 3.0
        // ----------------------------------------------------

        if (type == "v") {
            Vec3 vertex;

            ss
                >> vertex.x
                >> vertex.y
                >> vertex.z;

            vertices.push_back(vertex);
        }

        // ----------------------------------------------------
        // CARA
        //
        // El modelo fue exportado triangulado,
        // por lo que cada cara tiene tres vertices.
        //
        // Ejemplo:
        // f 1 2 3
        // ----------------------------------------------------

        else if (type == "f") {
            std::string s1;
            std::string s2;
            std::string s3;

            ss >> s1 >> s2 >> s3;

            if (
                s1.empty() ||
                s2.empty() ||
                s3.empty()
            ) {
                continue;
            }

            Face face {
                readIndex(
                    s1,
                    static_cast<int>(
                        vertices.size()
                    )
                ),

                readIndex(
                    s2,
                    static_cast<int>(
                        vertices.size()
                    )
                ),

                readIndex(
                    s3,
                    static_cast<int>(
                        vertices.size()
                    )
                )
            };

            faces.push_back(face);
        }
    }

    std::cout
        << "Vertices: "
        << vertices.size()
        << std::endl;

    std::cout
        << "Triangulos: "
        << faces.size()
        << std::endl;

    return
        !vertices.empty()
        &&
        !faces.empty();
}

// ============================================================
// OPERACIONES DE VECTORES
// ============================================================

Vec3 subtract(
    Vec3 a,
    Vec3 b
) {
    return Vec3 {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}

Vec3 cross(
    Vec3 a,
    Vec3 b
) {
    return Vec3 {
        a.y * b.z
            - a.z * b.y,

        a.z * b.x
            - a.x * b.z,

        a.x * b.y
            - a.y * b.x
    };
}

float dot(
    Vec3 a,
    Vec3 b
) {
    return
        a.x * b.x
        +
        a.y * b.y
        +
        a.z * b.z;
}

float length(
    Vec3 v
) {
    return std::sqrt(
        v.x * v.x
        +
        v.y * v.y
        +
        v.z * v.z
    );
}

Vec3 normalize(
    Vec3 v
) {
    float l =
        length(v);

    if (l < 0.0001f) {
        return Vec3 {
            0.0f,
            0.0f,
            1.0f
        };
    }

    return Vec3 {
        v.x / l,
        v.y / l,
        v.z / l
    };
}

// ============================================================
// ROTACIONES
// ============================================================

Vec3 rotateX(
    Vec3 v,
    float angle
) {
    float c =
        std::cos(angle);

    float s =
        std::sin(angle);

    return Vec3 {
        v.x,

        v.y * c
            - v.z * s,

        v.y * s
            + v.z * c
    };
}

Vec3 rotateZ(
    Vec3 v,
    float angle
) {
    float c =
        std::cos(angle);

    float s =
        std::sin(angle);

    return Vec3 {
        v.x * c
            - v.y * s,

        v.x * s
            + v.y * c,

        v.z
    };
}

// ============================================================
// RENDER
// ============================================================

void render() {
    clear();

    if (
        vertices.empty() ||
        faces.empty()
    ) {
        return;
    }

    // ========================================================
    // ENCONTRAR CENTRO DEL MODELO
    // ========================================================

    Vec3 minimum =
        vertices[0];

    Vec3 maximum =
        vertices[0];

    for (
        const Vec3& v :
        vertices
    ) {
        minimum.x =
            std::min(
                minimum.x,
                v.x
            );

        minimum.y =
            std::min(
                minimum.y,
                v.y
            );

        minimum.z =
            std::min(
                minimum.z,
                v.z
            );

        maximum.x =
            std::max(
                maximum.x,
                v.x
            );

        maximum.y =
            std::max(
                maximum.y,
                v.y
            );

        maximum.z =
            std::max(
                maximum.z,
                v.z
            );
    }

    Vec3 center {
        (
            minimum.x
            +
            maximum.x
        ) * 0.5f,

        (
            minimum.y
            +
            maximum.y
        ) * 0.5f,

        (
            minimum.z
            +
            maximum.z
        ) * 0.5f
    };

    // ========================================================
    // TRANSFORMAR VERTICES
    // ========================================================

    std::vector<Vec3>
        transformed;

    transformed.reserve(
        vertices.size()
    );

    // La nave se muestra principalmente desde arriba,
    // pero ligeramente inclinada para poder apreciar
    // también su volumen.

    const float PI =
        3.14159265f;

    const float angleX =
        25.0f
        * PI
        / 180.0f;

    const float angleZ =
        -12.0f
        * PI
        / 180.0f;

    for (
        const Vec3& original :
        vertices
    ) {
        // Centramos primero el modelo.
        Vec3 centered {
            original.x
                - center.x,

            original.y
                - center.y,

            original.z
                - center.z
        };

        // Blender y nuestra pantalla utilizan orientaciones
        // diferentes.
        //
        // Reordenamos los ejes para que la nave se vea
        // principalmente desde arriba.
        Vec3 v {
            centered.x,
            centered.z,
            centered.y
        };

        v =
            rotateZ(
                v,
                angleZ
            );

        v =
            rotateX(
                v,
                angleX
            );

        transformed.push_back(v);
    }

    // ========================================================
    // CALCULAR TAMANO DEL MODELO EN LA NUEVA ORIENTACION
    // ========================================================

    float minX =
        transformed[0].x;

    float maxX =
        transformed[0].x;

    float minY =
        transformed[0].y;

    float maxY =
        transformed[0].y;

    for (
        const Vec3& v :
        transformed
    ) {
        minX =
            std::min(
                minX,
                v.x
            );

        maxX =
            std::max(
                maxX,
                v.x
            );

        minY =
            std::min(
                minY,
                v.y
            );

        maxY =
            std::max(
                maxY,
                v.y
            );
    }

    float width =
        maxX - minX;

    float height =
        maxY - minY;

    if (width < 0.0001f) {
        width = 1.0f;
    }

    if (height < 0.0001f) {
        height = 1.0f;
    }

    // Queremos que el modelo ocupe aproximadamente
    // el 70% de la pantalla.

    float scaleX =
        (
            SCREEN_WIDTH
            * 0.70f
        )
        / width;

    float scaleY =
        (
            SCREEN_HEIGHT
            * 0.70f
        )
        / height;

    float scale =
        std::min(
            scaleX,
            scaleY
        );

    // ========================================================
    // PREPARAR TRIANGULOS
    // ========================================================

    std::vector<TriangleToDraw>
        triangles;

    triangles.reserve(
        faces.size()
    );

    // Luz direccional sencilla.
    Vec3 lightDirection =
        normalize(
            Vec3 {
                -0.4f,
                -0.5f,
                1.0f
            }
        );

    for (
        const Face& face :
        faces
    ) {
        // Validacion de indices.
        if (
            face.a < 0 ||
            face.b < 0 ||
            face.c < 0 ||
            face.a >=
                static_cast<int>(
                    transformed.size()
                ) ||
            face.b >=
                static_cast<int>(
                    transformed.size()
                ) ||
            face.c >=
                static_cast<int>(
                    transformed.size()
                )
        ) {
            continue;
        }

        // Recuperamos los tres vertices indicados
        // por los indices de la cara.
        Vec3 a =
            transformed[face.a];

        Vec3 b =
            transformed[face.b];

        Vec3 c =
            transformed[face.c];

        // ====================================================
        // NORMAL DEL TRIANGULO
        // ====================================================

        Vec3 edge1 =
            subtract(
                b,
                a
            );

        Vec3 edge2 =
            subtract(
                c,
                a
            );

        Vec3 normal =
            normalize(
                cross(
                    edge1,
                    edge2
                )
            );

        // ====================================================
        // ILUMINACION SENCILLA
        // ====================================================

        float brightness =
            0.30f
            +
            0.70f
            * std::abs(
                dot(
                    normal,
                    lightDirection
                )
            );

        brightness =
            std::clamp(
                brightness,
                0.30f,
                1.0f
            );

        // Azul similar al material usado en Blender.
        Color color(
            static_cast<uint8_t>(
                65
                * brightness
            ),

            static_cast<uint8_t>(
                130
                * brightness
            ),

            static_cast<uint8_t>(
                220
                * brightness
            ),

            255
        );

        // ====================================================
        // PROYECCION 3D -> 2D
        // ====================================================

        Vertex2D screenA {
            SCREEN_WIDTH
                * 0.5f
                +
                a.x
                * scale,

            SCREEN_HEIGHT
                * 0.5f
                -
                a.y
                * scale
        };

        Vertex2D screenB {
            SCREEN_WIDTH
                * 0.5f
                +
                b.x
                * scale,

            SCREEN_HEIGHT
                * 0.5f
                -
                b.y
                * scale
        };

        Vertex2D screenC {
            SCREEN_WIDTH
                * 0.5f
                +
                c.x
                * scale,

            SCREEN_HEIGHT
                * 0.5f
                -
                c.y
                * scale
        };

        // ====================================================
        // PROFUNDIDAD
        // ====================================================

        float depth =
            (
                a.z
                +
                b.z
                +
                c.z
            )
            / 3.0f;

        TriangleToDraw triangleData {
            screenA,
            screenB,
            screenC,

            depth,

            color
        };

        triangles.push_back(
            triangleData
        );
    }

    // ========================================================
    // PAINTER'S ALGORITHM
    //
    // Primero se dibujan los triangulos mas lejanos
    // y luego los mas cercanos.
    // ========================================================

    std::sort(
        triangles.begin(),
        triangles.end(),

        [](
            const TriangleToDraw& a,
            const TriangleToDraw& b
        ) {
            return
                a.depth
                <
                b.depth;
        }
    );

    // ========================================================
    // DIBUJAR TODOS LOS TRIANGULOS
    // ========================================================

    for (
        const TriangleToDraw& t :
        triangles
    ) {
        setColor(
            t.color
        );

        triangle(
            t.a,
            t.b,
            t.c
        );
    }
}

// ============================================================
// MAIN
// ============================================================

int main(
    int argc,
    char* argv[]
) {
    init();

    if (
        window == nullptr ||
        renderer == nullptr
    ) {
        SDL_Quit();

        return 1;
    }

    if (
        !loadOBJ(
            "models/nave.obj"
        )
    ) {
        SDL_DestroyRenderer(
            renderer
        );

        SDL_DestroyWindow(
            window
        );

        SDL_Quit();

        return 1;
    }

    bool running =
        true;

    while (running) {
        SDL_Event event;

        while (
            SDL_PollEvent(
                &event
            )
        ) {
            if (
                event.type
                ==
                SDL_QUIT
            ) {
                running =
                    false;
            }

            if (
                event.type
                    ==
                    SDL_KEYDOWN
                &&
                event.key.keysym.sym
                    ==
                    SDLK_ESCAPE
            ) {
                running =
                    false;
            }
        }

        render();

        RenderBuffer(
            renderer
        );

        SDL_RenderPresent(
            renderer
        );

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(
        renderer
    );

    SDL_DestroyWindow(
        window
    );

    SDL_Quit();

    return 0;
}