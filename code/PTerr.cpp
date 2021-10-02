
#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <intrin.h>
#include <SDL2/SDL.h>
//#undef main

#include "PTerr.h"

int PreviousFrameTime = 0;

inline __m128
mm_abs_ps(__m128 Val)
{
    __m128 NegMask = _mm_set1_ps(-0.0f);
    __m128 Result = _mm_andnot_ps(NegMask, Val);
    return Result;
}

static uint32_t
GetTotalPixelSize(image Image)
{
    uint32_t Result = Image.Width*Image.Height*sizeof(uint32_t);
    return Result;
}

static image
AllocateImage(uint32_t Width, uint32_t Height)
{
    image Image = {};
    Image.Width = Width;
    Image.Height = Height;
    uint32_t OutputPixelSize = GetTotalPixelSize(Image);
    Image.Pixels = (uint32_t*)malloc(OutputPixelSize);
    return Image;
}

static uint32_t
GetTotalMapSize(noise_map Map)
{
    uint32_t Result = Map.Width*Map.Height*sizeof(float);
    return Result;
}

static noise_map
AllocateNoiseMap(uint32_t Width, uint32_t Height)
{
    noise_map Map = {};
    Map.Width = Width;
    Map.Height = Height;
    uint32_t OutputPixelSize = GetTotalMapSize(Map);
    Map.Values = (float*)malloc(OutputPixelSize);
    return Map;
}

static void
WriteImages(image Image, char* OutputFileName)
{
    uint32_t OutputPixelSize = GetTotalPixelSize(Image);

    bitmap_header Header = {};
    Header.FileType = 0x4D42;
    Header.FileSize = sizeof(Header) + OutputPixelSize;
    Header.BitmapOffset = sizeof(Header);
    Header.Size = sizeof(Header) - 14;
    Header.Width = Image.Width;
    Header.Height = Image.Height;
    Header.Planes = 1;
    Header.BitsPerPixel = 32;
    Header.Compression = 0;
    Header.SizeOfBitmap = OutputPixelSize;
    Header.HorzResolution = 0;
    Header.VertResolution = 0;
    Header.ColorUsed = 0;
    Header.ColorImportant = 0;


    FILE* OutputFile;
    fopen_s(&OutputFile, OutputFileName, "wb");
    if(OutputFile)
    {
        fwrite(&Header, sizeof(Header), 1, OutputFile);
        fwrite(Image.Pixels, OutputPixelSize, 1, OutputFile);
        fclose(OutputFile); 
    }
    else
    {
        fprintf(stderr, "[ERROR] Unable to write to file %s.\n", OutputFileName);
    }
}

static world*
InitializeWorld()
{
    world* World = (world*)malloc(sizeof(world));

    World->ColorMap     = {};
    World->NoiseMap     = {};
    World->TerrainCount = 0;
    World->Terrains     = 0;
    World->MapChunkSize = 96;//241;
    //World->WorldMatrix  = GetIdentity();

    m4x4 Scale = ScaleMatrix(1.0f, 1.0f, 1.0f);
    m4x4 Translate = TranslateMatrix(GetIdentity(), V3(0, 0, 0));
    m4x4 Rotate = RotateInX(0);
    World->WorldMatrix = Scale*Translate*Rotate;

    return World;
}

static noise_map
GenerateNoiseMap(random_series* Series, int MapWidth, int MapHeight, float Scale, int Octaves, float Persistance, float Lacunarity, v2 Offset)
{
    noise_map Map = AllocateNoiseMap(MapWidth, MapHeight);

    v2* OctaveOffsets = (v2*)malloc(sizeof(v2)*Octaves);
    for(int OctaveIdx = 0; OctaveIdx < Octaves; ++OctaveIdx)
    {
        float OffsetX = RandomBetween(Series, -100000, 100000) + Offset.x;
        float OffsetY = RandomBetween(Series, -100000, 100000) + Offset.y;
        OctaveOffsets[OctaveIdx] = V2(OffsetX, OffsetY);
    }

    if(Scale <= 0.0f)
    {
        Scale = 0.00001f;
    }

    float MaxNoiseHeight = -FLT_MAX;
    float MinNoiseHeight =  FLT_MAX;

    float HalfWidth  = 0.5f * MapWidth;
    float HalfHeight = 0.5f * MapHeight;

    //float* Out = Map.Values;
    for(int32_t Y = 0; Y < MapHeight; ++Y)
    {
        for(int32_t X = 0; X < MapWidth; ++X)
        {
            float Amplitude = 1.0f;
            float Frequency = 1.0f;
            float NoiseHeight = 0;
            for(int OctaveIdx = 0; OctaveIdx < Octaves; ++OctaveIdx)
            {
                float SampleX = (X - HalfWidth) / Scale * Frequency + OctaveOffsets[OctaveIdx].x;
                float SampleY = (Y - HalfHeight) / Scale * Frequency + OctaveOffsets[OctaveIdx].y;
                float PerlinValue = 2.0f*PerlinNoise(SampleX, SampleY) - 1.0f;
                NoiseHeight += PerlinValue * Amplitude;

                Amplitude *= Persistance;
                Frequency *= Lacunarity;

            }
            if(NoiseHeight > MaxNoiseHeight)
            {
                MaxNoiseHeight = NoiseHeight;
            }
            else if(NoiseHeight < MinNoiseHeight)
            {
                MinNoiseHeight = NoiseHeight;
            }

            Map.Values[Y*Map.Width + X] = NoiseHeight;
        }
    }
    for(int32_t Y = 0; Y < MapHeight; ++Y)
    {
        for(int32_t X = 0; X < MapWidth; ++X)
        {
            Map.Values[Y*Map.Width + X] = InvLerp(MinNoiseHeight, Map.Values[Y*Map.Width + X], MaxNoiseHeight);
        }
    }

    return Map;
}

static uint32_t
CreateColor(v4 Color)
{
    uint32_t Result = (((uint32_t)roundf(Color.a * 255.0f) << 24) | 
                       ((uint32_t)roundf(Color.r * 255.0f) << 16) |
                       ((uint32_t)roundf(Color.g * 255.0f) <<  8) |
                       ((uint32_t)roundf(Color.b * 255.0f) <<  0));

    return Result;
}

static image
Pixelate(image* Image, int PixelSize = 7)
{
    int KernelSize = (PixelSize - 1) / 2;
#if 1
    image NewImage = AllocateImage(Image->Width, Image->Height);
    for(int Y = KernelSize; Y < (int32_t)Image->Height - KernelSize; Y += PixelSize)
    {
        for(int X = KernelSize; X < (int32_t)Image->Width; X += PixelSize)
        {
            uint32_t Color = Image->Pixels[Y*Image->Width + X];
            for(int KernelY = -KernelSize; KernelY <= KernelSize; ++KernelY)
            {
                for(int KernelX = -KernelSize; KernelX <= KernelSize; ++KernelX)
                {
                    NewImage.Pixels[(Y - KernelY)*(int32_t)Image->Width + (X - KernelX)] = Color;
                }
            }
        }
    }
#else
    image NewImage = AllocateImage(((Image->Width) / PixelSize), ((Image->Height) / PixelSize));
    for(int Y = 0; Y < (int32_t)NewImage.Height; ++Y)
    {
        for(int X = 0; X < (int32_t)NewImage.Width; ++X)
        {
            NewImage.Pixels[Y * NewImage.Width + X] = Image->Pixels[(Y + KernelSize)*NewImage.Width*PixelSize + (X + KernelSize)*PixelSize];
        }
    }
#endif
    return NewImage;
}

static terrain*
GetTerrain(world* World, uint32_t Index)
{
    terrain* Terrain = 0;
    if(Index < World->TerrainCount)
    {
        Terrain = World->Terrains + Index;
    }

    return Terrain;
}

static void
AddTerrain(world* World, terrain_type Type, v4 Color, float Height)
{
    terrain NewTerrain;

    NewTerrain.Color = Color;
    NewTerrain.Height = Height;
    NewTerrain.Type = Type;
    
    World->Terrains = (terrain*)realloc(World->Terrains, (World->TerrainCount + 1) * sizeof(terrain));

    World->Terrains[World->TerrainCount++] = NewTerrain;
}

static void
SwapTerrains(world* World, int Terr1, int Terr2)
{
    terrain TempTerrain    = World->Terrains[Terr1];
    World->Terrains[Terr1] = World->Terrains[Terr2];
    World->Terrains[Terr2] = TempTerrain;
}

static terrain
DeleteTerrain(world* World, int TerrIdx)
{
    terrain RetTerr = World->Terrains[TerrIdx];
    World->Terrains[TerrIdx] = World->Terrains[World->TerrainCount--];
    return RetTerr;
}

static void
InitializeMeshTerrain(world* World, int Width, int Height)
{
    World->Terrain.VerticesCount = Width * Height;
    World->Terrain.FacesCount    = (Width - 1)*(Height - 1)*2;
    World->Terrain.UVCount       = Width * Height;

    World->Terrain.Faces    = (face*)malloc(sizeof(face)*World->Terrain.FacesCount);
    World->Terrain.Vertices = (v3*)malloc(sizeof(v3)*World->Terrain.VerticesCount);
    World->Terrain.UVs      = (v2*)malloc(sizeof(v2)*World->Terrain.UVCount);
}

static void
AddTriangle(world* World, int* Index, int a, int b, int c)
{
    World->Terrain.Faces[*Index].a = a;
    World->Terrain.Faces[*Index].b = b;
    World->Terrain.Faces[*Index].c = c;

    *Index += 1;
}

static v2
HeightFlatten(float Height)
{
    v2 SmoothP1 = V2(0.0f,  0.0f);
    v2 SmoothP2 = V2(0.8f,  0.0f);
    v2 SmoothP3 = V2(0.8f, -0.1f);
    v2 SmoothP4 = V2(1.0f,  1.0f);

    v2 L0 = Lerp(SmoothP1, Height, SmoothP2);
    v2 L1 = Lerp(SmoothP2, Height, SmoothP3);
    v2 L2 = Lerp(SmoothP3, Height, SmoothP4);

    v2 Q0 = Lerp(L0, Height, L1);
    v2 Q1 = Lerp(L1, Height, L2);

    v2 Result = Lerp(Q0, Height, Q1);

    return Result;
}

static void
GenerateTerrainMesh(world* World, noise_map* NoiseMap, float HeightMultiplier, int LevelOfDetail = 0)
{
    int Width  = NoiseMap->Width;
    int Height = NoiseMap->Height;
    //float TopLeftX = (Width - 1)  / (-2.0f);
    //float TopLeftZ = (Height - 1) / ( 2.0f);

    if(LevelOfDetail < 0) LevelOfDetail = 0;
    if(LevelOfDetail > 6) LevelOfDetail = 6;

    int SimplificationInc = (LevelOfDetail == 0) ? 1 : (LevelOfDetail * 2);
    World->SimplificationLevel = SimplificationInc;
    int VerticesPerLine  = (Width - 1) / SimplificationInc + 1;

    InitializeMeshTerrain(World, VerticesPerLine, VerticesPerLine);

    int VertIndex = 0;
    int TrigIndex = 0;
    for(int Y = 0; Y < Height; Y += SimplificationInc)
    {
        for(int X = 0; X < Width; X += SimplificationInc)
        {
            World->Terrain.Vertices[VertIndex] = V3(1 + (float)X, HeightMultiplier * HeightFlatten(World->NoiseMap.Values[Y*World->NoiseMap.Width + X]).y * HeightMultiplier, 1 + (float)Y);
            World->Terrain.UVs[VertIndex]      = V2(X/(float)Width, Y/(float)Height);

            if((X < (Width - 1)) && (Y < (Height - 1)))
            {
                AddTriangle(World, &TrigIndex, VertIndex, VertIndex + VerticesPerLine + 1, VertIndex + VerticesPerLine);
                AddTriangle(World, &TrigIndex, VertIndex + VerticesPerLine + 1, VertIndex, VertIndex + 1);
            }

            VertIndex++;
        }
    }
}


static main_state* 
InitWindow(uint32_t Width, uint32_t Height, bool FullScreen = false)
{
    main_state* MainState = (main_state*)malloc(sizeof(main_state));
    MainState->ColorBuffer = (uint32_t*)malloc(sizeof(uint32_t)*Width*Height);

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_DisplayMode DisplayMode;
    SDL_GetCurrentDisplayMode(0, &DisplayMode);

    if(FullScreen)
    {
        MainState->WindowWidth = DisplayMode.w;
        MainState->WindowHeight = DisplayMode.h;
    }

    MainState->WindowWidth = Width;
    MainState->WindowHeight = Height;

    MainState->Window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width, Height, SDL_WINDOW_SHOWN);

    MainState->Renderer = SDL_CreateRenderer(MainState->Window, -1, SDL_RENDERER_ACCELERATED);

    MainState->Texture = SDL_CreateTexture(MainState->Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MainState->WindowWidth, MainState->WindowHeight);

    MainState->IsRunning = true;

    return MainState;
}

static void
DestroyWindow(main_state* MainState)
{
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
    SDL_DestroyTexture(MainState->Texture);
    SDL_DestroyRenderer(MainState->Renderer);
    SDL_DestroyWindow(MainState->Window);
    SDL_Quit();

    free(MainState->ColorBuffer);
    free(MainState);
}

static void
RenderColorBuffer(main_state* MainState)
{
    SDL_UpdateTexture(MainState->Texture, NULL, MainState->ColorBuffer, sizeof(uint32_t)*MainState->WindowWidth);
    SDL_RenderCopy(MainState->Renderer, MainState->Texture, NULL, NULL);
}

static v2
ProjectPoint(v3 Point, float FOW)
{
    v2 Result;

    Result.x = FOW * Point.x / Point.y;
    Result.y = FOW * Point.z / Point.y;

    return Result;
}

static void
PutPixel(main_state* MainState, uint32_t X, uint32_t Y, uint32_t Color)
{
    if((X >= 0) && (X < MainState->WindowWidth) && (Y >= 0) && (Y < MainState->WindowHeight))
    {
        MainState->ColorBuffer[Y*MainState->WindowWidth + X] = Color;
    }
}

static void
DrawRectangle(main_state* MainState, v2 Min, v2 Max, uint32_t Color)
{
    uint32_t MinX = (uint32_t)Min.x;
    uint32_t MinY = (uint32_t)Min.y;
    uint32_t MaxX = (uint32_t)Max.x;
    uint32_t MaxY = (uint32_t)Max.y;

    if(MinX < 0) {MinX = 0;}
    if(MinY < 0) {MinY = 0;}
    if(MaxX > MainState->WindowWidth) {MaxX = MainState->WindowWidth;}
    if(MaxY > MainState->WindowHeight) {MaxY = MainState->WindowHeight;}

    uint32_t Pitch = sizeof(uint32_t)*MainState->WindowWidth;

    uint8_t* Row = ((uint8_t*)MainState->ColorBuffer + MinY*Pitch + sizeof(uint32_t)*MinX);
    for(uint32_t Y = MinY; Y < MaxY; ++Y)
    {
        uint32_t* Pixel = (uint32_t*)Row;
        for(uint32_t X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color;
        }
        Row += Pitch;
    }
}

static void
ClearColorBuffer(main_state* MainState, uint32_t Color)
{
    DrawRectangle(MainState, V2(0, 0), V2((float)MainState->WindowWidth, (float)MainState->WindowHeight), Color);
}

void DrawLine(main_state* MainState, v2 Min, v2 Max, uint32_t Color)
{
    __m128 WindowWidth  = _mm_set1_ps(MainState->WindowWidth);
    __m128 WindowHeight = _mm_set1_ps(MainState->WindowHeight);
    __m128 Zero = _mm_set1_ps(0.0f);
    __m128 One  = _mm_set1_ps(1.0f);

    __m128 MinX = _mm_set1_ps(Min.x);
    __m128 MaxX = _mm_set1_ps(Max.x);
    __m128 MinY = _mm_set1_ps(Min.y);
    __m128 MaxY = _mm_set1_ps(Max.y);

    __m128 dX = _mm_sub_ps(MaxX, MinX);
    __m128 dY = _mm_sub_ps(MaxY, MinY); 

    __m128 SideLength = _mm_max_ps(mm_abs_ps(dX), mm_abs_ps(dY));

    __m128 IncX = _mm_div_ps(dX, SideLength);
    __m128 IncY = _mm_div_ps(dY, SideLength);

    __m128 CurrentX = MinX;
    __m128 CurrentY = MinY;

    for(int32_t PointIdx = 0; PointIdx <= ((float*)&SideLength)[0]; PointIdx++)
    {
        __m128 cmp0 = _mm_and_ps(_mm_cmpge_ps(CurrentX, Zero), _mm_cmplt_ps(CurrentX, WindowWidth));
        __m128 cmp1 = _mm_and_ps(_mm_cmpge_ps(CurrentY, Zero), _mm_cmplt_ps(CurrentY, WindowHeight));
        cmp0 = _mm_and_ps(cmp0, One);
        cmp1 = _mm_and_ps(cmp1, One);
        if (((float*)&cmp0)[0])
        {
            if (((float*)&cmp1)[0])
            {
                MainState->ColorBuffer[MainState->WindowWidth * (uint32_t)((float*)&CurrentY)[0] + (uint32_t)((float*)&CurrentX)[0]] = Color;
            }
        }

        CurrentX = _mm_add_ps(CurrentX, IncX);
        CurrentY = _mm_add_ps(CurrentY, IncY);
    }
}

static void
DrawTriangle(main_state* MainState, triangle Triangle, uint32_t Color)
{
    DrawLine(MainState, Triangle.Points[0], Triangle.Points[1], Color);
    DrawLine(MainState, Triangle.Points[1], Triangle.Points[2], Color);
    DrawLine(MainState, Triangle.Points[2], Triangle.Points[0], Color);
}

void DrawFilledTriangle(main_state* MainState, triangle Triangle, uint32_t Color)
{
    __m128 WindowWidth  = _mm_set1_ps(MainState->WindowWidth);
    __m128 WindowHeight = _mm_set1_ps(MainState->WindowHeight);
    __m128 Zero = _mm_set1_ps(0.0f);
    __m128 One  = _mm_set1_ps(1.0f);

    __m128 Ax  = _mm_set1_ps(Triangle.Points[0].x);
    __m128 Ay  = _mm_set1_ps(Triangle.Points[0].y);
    __m128 Bx  = _mm_set1_ps(Triangle.Points[1].x);
    __m128 By  = _mm_set1_ps(Triangle.Points[1].y);
    __m128 Cx  = _mm_set1_ps(Triangle.Points[2].x);
    __m128 Cy  = _mm_set1_ps(Triangle.Points[2].y);

    __m128 cmp0 = _mm_cmpgt_ps(Ay, By);
    if(((float*)&_mm_and_ps(cmp0, One))[0])
    {
        Ax = _mm_xor_ps(Ax, Bx);
        Bx = _mm_xor_ps(Bx, Ax);
        Ax = _mm_xor_ps(Ax, Bx);

        Ay = _mm_xor_ps(Ay, By);
        By = _mm_xor_ps(By, Ay);
        Ay = _mm_xor_ps(Ay, By);
    } // SwapVectors(&A, &B);
    __m128 cmp1 = _mm_cmpgt_ps(By, Cy);
    if(((float*)&_mm_and_ps(cmp1, One))[0])
    {
        Bx = _mm_xor_ps(Bx, Cx);
        Cx = _mm_xor_ps(Cx, Bx);
        Bx = _mm_xor_ps(Bx, Cx);
                          
        By = _mm_xor_ps(By, Cy);
        Cy = _mm_xor_ps(Cy, By);
        By = _mm_xor_ps(By, Cy);
    } // SwapVectors(&B, &C);
    cmp0 = _mm_cmpgt_ps(Ay, By);
    if(((float*)&_mm_and_ps(cmp0, One))[0])
    {
        Ax = _mm_xor_ps(Ax, Bx);
        Bx = _mm_xor_ps(Bx, Ax);
        Ax = _mm_xor_ps(Ax, Bx);
                          
        Ay = _mm_xor_ps(Ay, By);
        By = _mm_xor_ps(By, Ay);
        Ay = _mm_xor_ps(Ay, By);
    } // SwapVectors(&A, &B);

    cmp0 = _mm_and_ps(_mm_cmpeq_ps(Ay, By), One);
    cmp1 = _mm_and_ps(_mm_cmpeq_ps(By, Cy), One);
    if(((float*)&cmp1)[0]) 
    {
        __m128 Slope1 = _mm_div_ps(_mm_sub_ps(Bx, Ax), _mm_sub_ps(By, Ay));
        __m128 Slope2 = _mm_div_ps(_mm_sub_ps(Cx, Ax), _mm_sub_ps(Cy, Ay));

        __m128 StartX = Ax;
        __m128 EndX   = Ax;

        for(float Y = ((float*)&Ay)[0]; Y < ((float*)&Cy)[0]; ++Y)
        {
            __m128 MinX = StartX;
            __m128 MinY = _mm_set1_ps((float)Y);
            __m128 MaxX = EndX;
            __m128 MaxY = _mm_set1_ps((float)Y);

            __m128 dX = _mm_sub_ps(MaxX, MinX);
            __m128 dY = _mm_sub_ps(MaxY, MinY); 

            __m128 SideLength = _mm_max_ps(mm_abs_ps(dX), mm_abs_ps(dY));

            __m128 IncX = _mm_div_ps(dX, SideLength);
            __m128 IncY = _mm_div_ps(dY, SideLength);

            __m128 CurrentX = MinX;
            __m128 CurrentY = MinY;

            for(uint32_t PointIdx = 0; PointIdx < ((float*)&SideLength)[0]; ++PointIdx)
            {
                __m128 cmp0 = _mm_and_ps(_mm_cmpge_ps(CurrentX, Zero), _mm_cmplt_ps(CurrentX, WindowWidth));
                __m128 cmp1 = _mm_and_ps(_mm_cmpge_ps(CurrentY, Zero), _mm_cmplt_ps(CurrentY, WindowHeight));
                cmp0 = _mm_and_ps(cmp0, One);
                cmp1 = _mm_and_ps(cmp1, One);
                if (((float*)&cmp0)[0])
                {
                    if (((float*)&cmp1)[0])
                    {
                        MainState->ColorBuffer[MainState->WindowWidth * (uint32_t)((float*)&CurrentY)[0] + (uint32_t)((float*)&CurrentX)[0]] = Color;
                    }
                }

                CurrentX = _mm_add_ps(CurrentX, IncX);
                CurrentY = _mm_add_ps(CurrentY, IncY);
            }

            StartX = _mm_add_ps(StartX, Slope1);
            EndX   = _mm_add_ps(EndX,   Slope2);
        }
    } // FillFlatBottomTriangle(A, B, C, Color);
    else if(((float*)&cmp0)[0])
    {
        __m128 Slope1 = _mm_div_ps(_mm_sub_ps(Cx, Ax), _mm_sub_ps(Cy, Ay));
        __m128 Slope2 = _mm_div_ps(_mm_sub_ps(Cx, Bx), _mm_sub_ps(Cy, By));

        __m128 StartX = Cx;
        __m128 EndX   = Cx;

        for(float Y = ((float*)&Cy)[0]; Y > ((float*)&Ay)[0]; --Y)
        {
            __m128 MinX = StartX;
            __m128 MinY = _mm_set1_ps((float)Y);
            __m128 MaxX = EndX;
            __m128 MaxY = _mm_set1_ps((float)Y);

            __m128 dX = _mm_sub_ps(MaxX, MinX);
            __m128 dY = _mm_sub_ps(MaxY, MinY); 

            __m128 SideLength = _mm_max_ps(mm_abs_ps(dX), mm_abs_ps(dY));

            __m128 IncX = _mm_div_ps(dX, SideLength);
            __m128 IncY = _mm_div_ps(dY, SideLength);

            __m128 CurrentX = MinX;
            __m128 CurrentY = MinY;

            for(uint32_t PointIdx = 0; PointIdx < ((float*)&SideLength)[0]; ++PointIdx)
            {
                __m128 cmp0 = _mm_and_ps(_mm_cmpge_ps(CurrentX, Zero), _mm_cmplt_ps(CurrentX, WindowWidth));
                __m128 cmp1 = _mm_and_ps(_mm_cmpge_ps(CurrentY, Zero), _mm_cmplt_ps(CurrentY, WindowHeight));
                cmp0 = _mm_and_ps(cmp0, One);
                cmp1 = _mm_and_ps(cmp1, One);
                if (((float*)&cmp0)[0])
                {
                    if (((float*)&cmp1)[0])
                    {
                        MainState->ColorBuffer[MainState->WindowWidth * (uint32_t)((float*)&CurrentY)[0] + (uint32_t)((float*)&CurrentX)[0]] = Color;
                    }
                }

                CurrentX = _mm_add_ps(CurrentX, IncX);
                CurrentY = _mm_add_ps(CurrentY, IncY);
            }

            StartX = _mm_sub_ps(StartX, Slope1);
            EndX   = _mm_sub_ps(EndX, Slope2);
        }
    } // FillFlatTopTriangle(A, B, C, Color);
    else 
    {
        // Find Triangle Mid Point
        //v2 M = FindTriangleMid(A, B, C);
        __m128 My = By;
        __m128 Mx = _mm_add_ps(Ax, _mm_div_ps(_mm_mul_ps(_mm_sub_ps(By, Ay), _mm_sub_ps(Cx, Ax)), _mm_sub_ps(Cy, Ay)));

#if 1
        // Flat-bottom triangle
        // A = A, B = M, C = B
        __m128 Slope1 = _mm_div_ps(_mm_sub_ps(Bx, Ax), _mm_sub_ps(By, Ay));
        __m128 Slope2 = _mm_div_ps(_mm_sub_ps(Mx, Ax), _mm_sub_ps(My, Ay));

        __m128 StartX = Ax;
        __m128 EndX   = Ax;

        for(float Y = ((float*)&Ay)[0]; Y < ((float*)&My)[0]; ++Y)
        {
            __m128 MinX = StartX;
            __m128 MinY = _mm_set1_ps((float)Y);
            __m128 MaxX = EndX;
            __m128 MaxY = _mm_set1_ps((float)Y);

            __m128 dX = _mm_sub_ps(MaxX, MinX);
            __m128 dY = _mm_sub_ps(MaxY, MinY); 

            __m128 SideLength = _mm_max_ps(mm_abs_ps(dX), mm_abs_ps(dY));

            __m128 IncX = _mm_div_ps(dX, SideLength);
            __m128 IncY = _mm_div_ps(dY, SideLength);

            __m128 CurrentX = MinX;
            __m128 CurrentY = MinY;

            for(uint32_t PointIdx = 0; PointIdx < ((float*)&SideLength)[0]; ++PointIdx)
            {
                __m128 cmp0 = _mm_and_ps(_mm_cmpge_ps(CurrentX, Zero), _mm_cmplt_ps(CurrentX, WindowWidth));
                __m128 cmp1 = _mm_and_ps(_mm_cmpge_ps(CurrentY, Zero), _mm_cmplt_ps(CurrentY, WindowHeight));
                cmp0 = _mm_and_ps(cmp0, One);
                cmp1 = _mm_and_ps(cmp1, One);
                if (((float*)&cmp0)[0])
                {
                    if (((float*)&cmp1)[0])
                    {
                        MainState->ColorBuffer[MainState->WindowWidth * (uint32_t)((float*)&CurrentY)[0] + (uint32_t)((float*)&CurrentX)[0]] = Color;
                    }
                }

                CurrentX = _mm_add_ps(CurrentX, IncX);
                CurrentY = _mm_add_ps(CurrentY, IncY);
            }

            StartX = _mm_add_ps(StartX, Slope1);
            EndX   = _mm_add_ps(EndX,   Slope2);
        }

        // Flat-top triangle
        // A = B, B = M, C = C
        Slope1 = _mm_div_ps(_mm_sub_ps(Cx, Bx), _mm_sub_ps(Cy, By));
        Slope2 = _mm_div_ps(_mm_sub_ps(Cx, Mx), _mm_sub_ps(Cy, My));

        StartX = Cx;
        EndX   = Cx;

        for(float Y = ((float*)&Cy)[0]; Y > ((float*)&By)[0]; --Y)
        {
            __m128 MinX = StartX;
            __m128 MinY = _mm_set1_ps((float)Y);
            __m128 MaxX = EndX;
            __m128 MaxY = _mm_set1_ps((float)Y);

            __m128 dX = _mm_sub_ps(MaxX, MinX);
            __m128 dY = _mm_sub_ps(MaxY, MinY); 

            __m128 SideLength = _mm_max_ps(mm_abs_ps(dX), mm_abs_ps(dY));

            __m128 IncX = _mm_div_ps(dX, SideLength);
            __m128 IncY = _mm_div_ps(dY, SideLength);

            __m128 CurrentX = MinX;
            __m128 CurrentY = MinY;

            for(uint32_t PointIdx = 0; PointIdx < ((float*)&SideLength)[0]; ++PointIdx)
            {
                __m128 cmp0 = _mm_and_ps(_mm_cmpge_ps(CurrentX, Zero), _mm_cmplt_ps(CurrentX, WindowWidth));
                __m128 cmp1 = _mm_and_ps(_mm_cmpge_ps(CurrentY, Zero), _mm_cmplt_ps(CurrentY, WindowHeight));
                cmp0 = _mm_and_ps(cmp0, One);
                cmp1 = _mm_and_ps(cmp1, One);
                if (((float*)&cmp0)[0])
                {
                    if (((float*)&cmp1)[0])
                    {
                        MainState->ColorBuffer[MainState->WindowWidth * (uint32_t)((float*)&CurrentY)[0] + (uint32_t)((float*)&CurrentX)[0]] = Color;
                    }
                }

                CurrentX = _mm_add_ps(CurrentX, IncX);
                CurrentY = _mm_add_ps(CurrentY, IncY);
            }

            StartX = _mm_sub_ps(StartX, Slope1);
            EndX   = _mm_sub_ps(EndX,   Slope2);
        }
#endif
    }
}

static void 
ApplyTexture(main_state* MainState, world* World)
{
    float Width  = (World->TerrainTexture.Width  - 1) / World->SimplificationLevel;
    float Height = (World->TerrainTexture.Height - 1) / World->SimplificationLevel;
    for(uint32_t Y = 0, MeshIdx = 1; Y < Height; ++Y)
    {
        for(uint32_t X = 0; X < Width; ++X)
        {
            uint32_t Color = World->TerrainTexture.Pixels[Y*World->TerrainTexture.Width*World->SimplificationLevel + X*World->SimplificationLevel];

            triangle Triangle1;
            triangle Triangle2;

            Triangle1.Points[0] = xz(MainState->World->Terrain.Vertices[MainState->World->Terrain.Faces[MeshIdx - 1].a]);
            Triangle1.Points[1] = xz(MainState->World->Terrain.Vertices[MainState->World->Terrain.Faces[MeshIdx - 1].b]);
            Triangle1.Points[2] = xz(MainState->World->Terrain.Vertices[MainState->World->Terrain.Faces[MeshIdx - 1].c]);

            Triangle2.Points[0] = xz(MainState->World->Terrain.Vertices[MainState->World->Terrain.Faces[MeshIdx].a]);
            Triangle2.Points[1] = xz(MainState->World->Terrain.Vertices[MainState->World->Terrain.Faces[MeshIdx].b]);
            Triangle2.Points[2] = xz(MainState->World->Terrain.Vertices[MainState->World->Terrain.Faces[MeshIdx].c]);
            MeshIdx += 2;

            DrawFilledTriangle(MainState, Triangle1, Color);
            DrawFilledTriangle(MainState, Triangle2, Color);
        }
    }
}

static void 
ProcessInput(main_state* MainState)
{
    SDL_Event event;
    SDL_PollEvent(&event);

    switch(event.type)
    {
        case SDL_QUIT:
            {
                MainState->IsRunning = false;
            }break;
        case SDL_KEYDOWN:
            {
                if(event.key.keysym.sym == SDLK_ESCAPE) MainState->IsRunning = false;
                if(event.key.keysym.sym == SDLK_w) {
                    m4x4 Move = TranslateMatrix(GetIdentity(), V3(0, 0, -1)); 
                    MainState->World->WorldMatrix = MainState->World->WorldMatrix*Move;
                }
                if(event.key.keysym.sym == SDLK_a) 
                {
                    m4x4 Move = TranslateMatrix(GetIdentity(), V3(-1, 0, 0)); 
                    MainState->World->WorldMatrix = MainState->World->WorldMatrix*Move;
                }
                if(event.key.keysym.sym == SDLK_s) 
                {
                    m4x4 Move = TranslateMatrix(GetIdentity(), V3(0, 0, 1)); 
                    MainState->World->WorldMatrix = MainState->World->WorldMatrix*Move;
                }
                if(event.key.keysym.sym == SDLK_d) 
                {
                    m4x4 Move = TranslateMatrix(GetIdentity(), V3(1, 0, 0)); 
                    MainState->World->WorldMatrix = MainState->World->WorldMatrix*Move;
                }
                if(event.key.keysym.sym == SDLK_r) 
                {
                    m4x4 Move = RotateInX(0.001);//TranslateMatrix(GetIdentity(), V3(0, 1, 0)); 
                    v3 CenterPoint = V3(0.5f*MainState->World->MapChunkSize, 0.0f, 0.5f*MainState->World->MapChunkSize);
                    v3 NegCenterPoint = V3(-0.5f*MainState->World->MapChunkSize, 0, -0.5f*MainState->World->MapChunkSize);
                    m4x4 Trans = TranslateMatrix(GetIdentity(), CenterPoint);
                    MainState->World->WorldMatrix = MainState->World->WorldMatrix*TranslateMatrix(GetIdentity(), CenterPoint)*Move*TranslateMatrix(GetIdentity(), NegCenterPoint);
                }
                if(event.key.keysym.sym == SDLK_f) 
                {
                    m4x4 Move = RotateInX(-0.001);//TranslateMatrix(GetIdentity(), V3(0, -1, 0)); 
                    v3 CenterPoint = V3(0.5f*MainState->World->MapChunkSize, 0, 0.5f*MainState->World->MapChunkSize);
                    v3 NegCenterPoint = V3(-0.5f*MainState->World->MapChunkSize, 0, -0.5f*MainState->World->MapChunkSize);
                    m4x4 Trans = TranslateMatrix(GetIdentity(), CenterPoint);
                    MainState->World->WorldMatrix = MainState->World->WorldMatrix*TranslateMatrix(GetIdentity(), CenterPoint)*Move*TranslateMatrix(GetIdentity(), NegCenterPoint);
                }
                if(event.key.keysym.sym == SDLK_UP)
                {
                    m4x4 Move = ScaleMatrix(1.01, 1.01, 1.01);
                    MainState->World->WorldMatrix = MainState->World->WorldMatrix*Move;
                }
                if(event.key.keysym.sym == SDLK_DOWN)
                {
                    m4x4 Move = ScaleMatrix(1/1.01, 1/1.01, 1/1.01);
                    MainState->World->WorldMatrix = MainState->World->WorldMatrix*Move;
                }
            }
            break;
    }
}

static void
UpdateMesh(main_state* MainState)
{
    for(uint32_t VertIdx = 0; VertIdx < MainState->World->Terrain.VerticesCount; ++VertIdx)
    {
        v3 Vert = MainState->World->Terrain.Vertices[VertIdx];
        Vert = MainState->World->WorldMatrix*Vert;
        MainState->World->Terrain.Vertices[VertIdx] = Vert;
    }
}

static void
Update(main_state* MainState)
{
    int TimeToWait = FRAME_TARGET_TIME - (SDL_GetTicks() + PreviousFrameTime);

    if(TimeToWait > 0 && (TimeToWait <= FRAME_TARGET_TIME))
    {
        SDL_Delay(TimeToWait);
    }

    for(uint32_t VertIdx = 0; VertIdx < MainState->World->Terrain.VerticesCount; ++VertIdx)
    {
        v3 Vert = MainState->World->Terrain.Vertices[VertIdx];
        Vert = MainState->World->WorldMatrix*Vert;
        MainState->World->Terrain.Vertices[VertIdx] = Vert;
    }
}

static void
Render(main_state* MainState)
{
    for(uint32_t FaceIdx = 0; FaceIdx < (MainState->World->Terrain.FacesCount); FaceIdx += 1)
    {
        triangle Triangle;

        Triangle.Points[0] = xz(MainState->World->Terrain.Vertices[MainState->World->Terrain.Faces[FaceIdx].a]);
        Triangle.Points[1] = xz(MainState->World->Terrain.Vertices[MainState->World->Terrain.Faces[FaceIdx].b]);
        Triangle.Points[2] = xz(MainState->World->Terrain.Vertices[MainState->World->Terrain.Faces[FaceIdx].c]);

        DrawTriangle(MainState, Triangle, CreateColor(V4(0.0f, 0.0f, 0.0f, 1.0f)));
    }
    
    ApplyTexture(MainState, MainState->World);

    RenderColorBuffer(MainState);
    ClearColorBuffer(MainState, CreateColor(V4(0.529411765, 0.807843137, 0.921568627, 1.0f)));
    SDL_RenderPresent(MainState->Renderer);
}

int main(int argc, char** argv)
{
    random_series Series = RandomSeed(523);

    main_state* MainState = InitWindow(1280, 720);

    MainState->World = InitializeWorld();

    MainState->World->NoiseMap = GenerateNoiseMap(&Series, MainState->World->MapChunkSize, MainState->World->MapChunkSize, 4.0f, 4, 0.5f, 2.0f, V2(5, 0));
    GenerateTerrainMesh(MainState->World, &MainState->World->NoiseMap, 5.0f);

    AddTerrain(MainState->World, TerrainType_BlueWater, V4(9, 4, 143, 1), 0.30f);
    AddTerrain(MainState->World, TerrainType_Water, V4(0, 181, 204, 1), 0.35f);
    AddTerrain(MainState->World, TerrainType_Sand, V4(207, 174, 117, 1), 0.40f);
    AddTerrain(MainState->World, TerrainType_Land1, V4(0, 194, 0, 1), 0.55f);
    AddTerrain(MainState->World, TerrainType_Land2, V4(9, 92, 9, 1), 0.60f);
    AddTerrain(MainState->World, TerrainType_Rock1, V4(158, 76, 17, 1), 0.70f);
    AddTerrain(MainState->World, TerrainType_Rock2, V4(89, 38, 2, 1), 0.85f);
    AddTerrain(MainState->World, TerrainType_Snow, V4(255, 255, 255, 1), 1.0f);

    
    MainState->World->ColorMap = AllocateImage(MainState->World->NoiseMap.Width, MainState->World->NoiseMap.Height);
    for(uint32_t Y = 0; Y < MainState->World->MapChunkSize; ++Y)
    {
        for(uint32_t X = 0; X < MainState->World->MapChunkSize; ++X)
        {
            float CurrentHeight = MainState->World->NoiseMap.Values[Y * (int32_t)MainState->World->NoiseMap.Width + X];
            for(uint32_t TerrainIdx = 0; TerrainIdx < MainState->World->TerrainCount; ++TerrainIdx)
            {
                if(CurrentHeight <= MainState->World->Terrains[TerrainIdx].Height)
                {
                    MainState->World->ColorMap.Pixels[Y*MainState->World->ColorMap.Width + X] = BGRAPack4x8(MainState->World->Terrains[TerrainIdx].Color);
                    break;
                }
            }
        }
    }
    MainState->World->TerrainTexture = Pixelate(&MainState->World->ColorMap, MainState->World->SimplificationLevel);
    

    UpdateMesh(MainState);
    while(MainState->IsRunning)
    {
        ProcessInput(MainState);
        Update(MainState);
        Render(MainState);
    }

    /*
    FILE* OutputFile;
    fopen_s(&OutputFile, "GeneratedVerticesData.txt", "w+");
    if(OutputFile)
    {
        for(int i = 0; i < MainState->World->Terrain.VerticesCount; ++i)
        {
            v3 Vert = MainState->World->Terrain.Vertices[i];
            fprintf(OutputFile, "(%f, %f, %f) \n", Vert.x, Vert.y, Vert.z);
        }
        fclose(OutputFile); 
    }
    fopen_s(&OutputFile, "GeneratedMeshData.txt", "w");
    if(OutputFile)
    {
        for(int i = 0; i < MainState->World->Terrain.FacesCount; ++i)
        {
            face Face = MainState->World->Terrain.Faces[i];
            fprintf(OutputFile, "%i %i %i \n", Face.a, Face.b, Face.c);
        }
        fclose(OutputFile);
    }
    */

    DestroyWindow(MainState);    
    //WriteImages(World->ColorMap, "test.bmp");
    WriteImages(MainState->World->TerrainTexture, "PixelateTest.bmp");
    return 0;
}

