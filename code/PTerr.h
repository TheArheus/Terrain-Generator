#if !defined(PTERR_H_)
#define PTERR_H_

#include "thmath.h"
#include "hrandom.h"
#include "Noise.h"

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))
#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

#define FPS 30
#define FRAME_TARGET_TIME (1000/FPS)

enum terrain_type
{
    TerrainType_BlueWater = (1 << 0),
    TerrainType_Water = (1 << 1),
    TerrainType_Sand  = (1 << 2),
    TerrainType_Land1 = (1 << 3),
    TerrainType_Land2 = (1 << 4),
    TerrainType_Rock1 = (1 << 5),
    TerrainType_Rock2 = (1 << 6),
    TerrainType_Snow  = (1 << 7),
};

#pragma pack(push, 1)
struct bitmap_header
{
    uint16_t FileType;
    uint32_t FileSize;
    uint16_t Reserved1;
    uint16_t Reserved2;
    uint32_t BitmapOffset;
    uint32_t Size;
    int32_t  Width;
    int32_t  Height;
    uint16_t Planes;
    uint16_t BitsPerPixel;
    uint32_t Compression;
    uint32_t SizeOfBitmap;
    int32_t  HorzResolution;
    int32_t  VertResolution;
    uint32_t ColorUsed;
    uint32_t ColorImportant;
};
#pragma pack(pop)

struct image
{
    uint32_t Width;
    uint32_t Height;
    uint32_t* Pixels;
};

struct noise_map
{
    uint32_t Width;
    uint32_t Height;
    float* Values;
};

struct terrain
{
    terrain_type Type;
    float Height;
    v4 Color;
};

union face
{
    struct
    {
        int a, b, c;
    };
    int E[3];
};

struct triangle
{
    v2 Points[3];
    uint32_t Color;
};

struct mesh_terrain
{
    uint32_t VerticesCount;
    v3* Vertices;

    uint32_t FacesCount;
    face* Faces;

    uint32_t UVCount;
    v2* UVs;
};

struct world
{
    terrain* Terrains;
    uint32_t TerrainCount;
    image TerrainTexture;

    image ColorMap;
    noise_map NoiseMap;

    mesh_terrain Terrain;

    uint32_t MapChunkSize;
    uint32_t SimplificationLevel;

    m4x4 WorldMatrix;
};

struct main_state
{
    SDL_Window* Window;
    SDL_Renderer* Renderer;
    SDL_Texture* Texture;

    world* World;

    uint32_t* ColorBuffer;

    uint32_t WindowWidth;
    uint32_t WindowHeight;

    uint32_t IsRunning;
};

#endif
