
union v2
{
    struct
    {
        float x, y;
    };
    float E[2];
};

union v3
{
    struct
    {
        float x, y, z;
    };
    struct
    {
        float r, g, b;
    };
    struct
    {
        v2 xy;
        float Ignored0_;
    };
    struct
    {
        float Ignored1_;
        v2 yz;
    };
    float E[3];
};

union v4
{
    struct
    {
        union
        {
            v3 xyz;
            struct
            {
                float x, y, z;
            };
        };

        float w;
    };
    struct
    {
        union
        {
            v3 rgb;
            struct
            {
                float r, g, b;
            };
        };

        float a;
    };
    float E[4];
};

struct m4x4
{
    float E[4][4];
};

//
// Vector 2
//

inline v2
V2(float X, float Y)
{
    v2 Result;

    Result.x = X;
    Result.y = Y;

    return Result;
}

#define xz(Vector) (V2(Vector.x, Vector.z))

inline v2
operator+(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return Result;
}

inline v2
operator-(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return Result;
}
inline v2
operator-(v2 A, float B)
{
    v2 Result;

    Result.x = A.x - B;
    Result.y = A.y - B;

    return Result;
}

inline v2
operator-(float A, v2 B)
{
    v2 Result = B - A;
    return Result;
}

inline v2
operator*(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;

    return Result;
}

inline v2
operator*(float A, v2 B)
{
    v2 Result;

    Result.x = A * B.x;
    Result.y = A * B.y;

    return Result;
}

inline v2
Lerp(v2 A, float t, v2 B)
{
    v2 Result = (1.0f - t)*A + t*B;
    return Result;
}

//
// Vector 3
//

inline v3
V3(float X, float Y, float Z)
{
    v3 Result;

    Result.x = X;
    Result.y = Y;
    Result.z = Z;

    return Result;
}

inline v3
operator+(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;

    return Result;
}

inline v3
operator-(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;

    return Result;
}

inline v3
operator*(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;

    return Result;
}

inline v3
operator*(float A, v3 B)
{
    v3 Result;

    Result.x = A * B.x;
    Result.y = A * B.y;
    Result.z = A * B.z;

    return Result;
}

inline v3
operator/(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x / B.x;
    Result.y = A.y / B.y;
    Result.z = A.z / B.z;

    return Result;
}

inline v3
operator/(float A, v3 B)
{
    v3 Result;

    Result.x = A / B.x;
    Result.y = A / B.y;
    Result.z = A / B.z;

    return Result;
}

inline v3
operator/(v3 A, float B)
{
    v3 Result = B / A;

    return Result;
}

//
// Vector 4
//

inline v4
V4(float X, float Y, float Z, float W)
{
    v4 Result;

    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    Result.w = W;

    return Result;
}

inline v4
V4(v3 XYZ, float W)
{
    v4 Result;

    Result.xyz = XYZ;
    Result.w = W;

    return Result;
}

inline v4
operator+(v4 A, v4 B)
{
    v4 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    Result.w = A.w + B.w;

    return Result;
}

inline v4
operator-(v4 A, v4 B)
{
    v4 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    Result.w = A.w - B.w;

    return Result;
}
inline v4
operator-(v4 A, float B)
{
    v4 Result;

    Result.x = A.x - B;
    Result.y = A.y - B;
    Result.z = A.z - B;
    Result.w = A.w - B;

    return Result;
}

inline v4
operator-(float A, v4 B)
{
    v4 Result = B - A;
    return Result;
}

inline v4
operator*(v4 A, v4 B)
{
    v4 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;
    Result.w = A.w * B.w;

    return Result;
}

inline v4
operator*(float A, v4 B)
{
    v4 Result;

    Result.x = A * B.x;
    Result.y = A * B.y;
    Result.z = A * B.z;
    Result.w = A * B.w;

    return Result;
}

inline v4
operator/(v4 A, v4 B)
{
    v4 Result;

    Result.x = A.x / B.x;
    Result.y = A.y / B.y;
    Result.z = A.z / B.z;
    Result.w = A.w / B.w;

    return Result;
}

inline v4
operator/(float A, v4 B)
{
    v4 Result;

    Result.x = A / B.x;
    Result.y = A / B.y;
    Result.z = A / B.z;
    Result.w = A / B.w;

    return Result;
}

inline v4
operator/(v4 A, float B)
{
    v4 Result = B / A;

    return Result;
}

inline v4
operator*(v4 A, float B)
{
    v4 Result = B * A;

    return Result;
}

inline v4
Lerp(v4 A, float t, v4 B)
{
    v4 Result = (1.0f - t)*A + t*B;
    return Result;
}

static v4
InvLerp(v4 A, float t, v4 B)
{
    v4 Result = (t - A) / (B - A);
    return Result;
}

//
// Scalar 
//

static float
Lerp(float A, float t, float B)
{
    float Result = (1 - t)*A + t*B;
    return Result;
}

static float
InvLerp(float A, float t, float B)
{
    float Result = (t - A) / (B - A);
    return Result;
}

//
// Color
//

inline v4
RGBAToLinear1(v4 A)
{
    float Inv255 = 1.0f / 255.0f;

    v4 Result = Inv255 * A;

    return Result;
}

inline v4
Linear1ToRGBA(v4 A)
{
    float Val255 = 255.0f;

    v4 Result = Val255 * A;

    return Result;
}

inline v4
RGBAUnpack4x8(uint32_t Packed)
{
    v4 Result = {(float)((Packed >>  0) & 0xFF),
                 (float)((Packed >>  8) & 0xFF),
                 (float)((Packed >> 16) & 0xFF),
                 (float)((Packed >> 24) & 0xFF)};

    return(Result);
}

inline uint32_t
BGRAPack4x8(v4 Unpacked)
{
    uint32_t Result = (((uint32_t)roundf(Unpacked.a) << 24) |
                       ((uint32_t)roundf(Unpacked.r) << 16) |
                       ((uint32_t)roundf(Unpacked.g) <<  8) |
                       ((uint32_t)roundf(Unpacked.b) <<  0));
    
    return(Result);
}

inline v4
BGRAUnpack4x8(uint32_t Packed)
{
    v4 Result = V4((float)((Packed >> 16) & 0xFF), 
                   (float)((Packed >>  8) & 0xFF), 
                   (float)((Packed >>  0) & 0xFF), 
                   (float)((Packed >> 24) & 0xFF));
    
    return(Result);
}

//
// Matrix 4x4
//

inline m4x4
operator*(m4x4 A, m4x4 B)
{
    m4x4 Result = {};
    for(int r = 0; r <= 3; ++r)
    {
        for(int c = 0; c <= 3; ++c)
        {
            for(int i = 0; i <= 3; ++i)
            {
                Result.E[r][c] += A.E[r][i] * B.E[i][c];
            }
        }
    }

    return Result;
}

v3 Transform(m4x4 M, v3 P, float Pw = 1.0f)
{
    v3 Result;

    Result.E[0] = P.x*M.E[0][0] + P.y*M.E[0][1] + P.z*M.E[0][2] + Pw*M.E[0][3]; 
    Result.E[1] = P.x*M.E[1][0] + P.y*M.E[1][1] + P.z*M.E[1][2] + Pw*M.E[1][3]; 
    Result.E[2] = P.x*M.E[2][0] + P.y*M.E[2][1] + P.z*M.E[2][2] + Pw*M.E[2][3]; 

    return Result;
}

inline v3 
operator*(m4x4 A, v3 B)
{
    v3 Result;
    Result = Transform(A, B, 1.0f);
    return Result;
}
                                  
inline m4x4                       
GetIdentity()
{
    m4x4 Result = 
    {
        {
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        }
    };

    return Result;
}

inline m4x4
ScaleMatrix(float x, float y, float z)
{
    m4x4 Result = GetIdentity();

    Result.E[0][0] = x;
    Result.E[1][1] = y;
    Result.E[2][2] = z;

    return Result;
}

inline m4x4
ScaleMatrix(v3 P)
{
    m4x4 Result = GetIdentity();

    Result.E[0][0] = P.x;
    Result.E[1][1] = P.y;
    Result.E[2][2] = P.z;

    return Result;
}

inline m4x4
TranslateMatrix(m4x4 M, v3 P)
{
    m4x4 Result = M;

    Result.E[0][3] += P.x;
    Result.E[1][3] += P.y;
    Result.E[2][3] += P.z;

    return Result;
}

m4x4
RotateInX(float V)
{
    float s = sinf(V);
    float c = cosf(V);

    m4x4 Result = 
    {
        {
            {1, 0,  0, 0},
            {0, c, -s, 0},
            {0, s,  c, 0},
            {0, 0,  0, 1},
        }
    };

    return Result;
}

m4x4
RotateInY(float V)
{
    float s = sinf(V);
    float c = cosf(V);

    m4x4 Result = 
    {
        {
            {c, 0, -s, 0},
            {0, 1,  0, 0},
            {s, 0,  c, 0},
            {0, 0,  0, 1},
        }
    };

    return Result;
}

m4x4
RotateInZ(float V)
{
    float s = sinf(V);
    float c = cosf(V);

    m4x4 Result = 
    {
        {
            {c, -s, 0, 0},
            {s,  c, 0, 0},
            {0,  0, 1, 0},
            {0,  0, 0, 1},
        }
    };

    return Result;
}
