#pragma once
// =============================================================================
// sdl2_compat.hpp - SDL2 compatibility layer for th06
// Replaces all D3D8, DirectInput, DirectSound, Win32 types and functions
// =============================================================================

#include <SDL.h>
#include <SDL_opengl.h>
#include <cmath>
#include <cstring>
#include "inttypes.hpp"

// =============================================================================
// D3DCOLOR: ARGB packed u32 (same layout as original)
// =============================================================================
typedef u32 D3DCOLOR;

#define D3DCOLOR_ARGB(a,r,g,b) ((D3DCOLOR)(((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)
#define D3DCOLOR_XRGB(r,g,b)  D3DCOLOR_ARGB(0xff,r,g,b)

inline u8 D3DCOLOR_A(D3DCOLOR c) { return (u8)((c >> 24) & 0xff); }
inline u8 D3DCOLOR_R(D3DCOLOR c) { return (u8)((c >> 16) & 0xff); }
inline u8 D3DCOLOR_G(D3DCOLOR c) { return (u8)((c >> 8) & 0xff); }
inline u8 D3DCOLOR_B(D3DCOLOR c) { return (u8)(c & 0xff); }

// =============================================================================
// Vector / Matrix types (replace D3DXVECTOR2/3/4, D3DXMATRIX)
// =============================================================================
struct D3DXVECTOR2
{
    f32 x, y;
    D3DXVECTOR2() : x(0), y(0) {}
    D3DXVECTOR2(f32 _x, f32 _y) : x(_x), y(_y) {}
    D3DXVECTOR2 operator+(const D3DXVECTOR2 &v) const { return {x + v.x, y + v.y}; }
    D3DXVECTOR2 operator-(const D3DXVECTOR2 &v) const { return {x - v.x, y - v.y}; }
    D3DXVECTOR2 operator*(f32 s) const { return {x * s, y * s}; }
    D3DXVECTOR2 &operator+=(const D3DXVECTOR2 &v) { x += v.x; y += v.y; return *this; }
    D3DXVECTOR2 &operator-=(const D3DXVECTOR2 &v) { x -= v.x; y -= v.y; return *this; }
    D3DXVECTOR2 operator/(f32 s) const { return {x / s, y / s}; }
    operator f32*() { return &x; }
    operator const f32*() const { return &x; }
};

struct D3DXVECTOR3
{
    f32 x, y, z;
    D3DXVECTOR3() : x(0), y(0), z(0) {}
    D3DXVECTOR3(f32 _x, f32 _y, f32 _z) : x(_x), y(_y), z(_z) {}
    D3DXVECTOR3 operator+(const D3DXVECTOR3 &v) const { return {x + v.x, y + v.y, z + v.z}; }
    D3DXVECTOR3 operator-(const D3DXVECTOR3 &v) const { return {x - v.x, y - v.y, z - v.z}; }
    D3DXVECTOR3 operator-() const { return {-x, -y, -z}; }
    D3DXVECTOR3 operator*(f32 s) const { return {x * s, y * s, z * s}; }
    D3DXVECTOR3 &operator+=(const D3DXVECTOR3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
    D3DXVECTOR3 &operator-=(const D3DXVECTOR3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    D3DXVECTOR3 &operator*=(f32 s) { x *= s; y *= s; z *= s; return *this; }
    D3DXVECTOR3 operator/(f32 s) const { return {x / s, y / s, z / s}; }
    D3DXVECTOR3 &operator/=(f32 s) { x /= s; y /= s; z /= s; return *this; }
    f32 &operator[](int i) { return (&x)[i]; }
    f32 operator[](int i) const { return (&x)[i]; }
    operator f32*() { return &x; }
    operator const f32*() const { return &x; }
};

struct D3DXVECTOR4
{
    f32 x, y, z, w;
    D3DXVECTOR4() : x(0), y(0), z(0), w(0) {}
    D3DXVECTOR4(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w) {}
};

typedef D3DXVECTOR4 D3DXQUATERNION;

inline D3DXVECTOR3 operator*(f32 s, const D3DXVECTOR3 &v) { return v * s; }
inline D3DXVECTOR2 operator*(f32 s, const D3DXVECTOR2 &v) { return v * s; }

struct D3DXMATRIX
{
    union {
        struct {
            f32 _11, _12, _13, _14;
            f32 _21, _22, _23, _24;
            f32 _31, _32, _33, _34;
            f32 _41, _42, _43, _44;
        };
        f32 m[4][4];
    };
    D3DXMATRIX()
    {
        memset(m, 0, sizeof(m));
    }
};

inline D3DXMATRIX *D3DXMatrixIdentity(D3DXMATRIX *out)
{
    memset(out->m, 0, sizeof(out->m));
    out->_11 = out->_22 = out->_33 = out->_44 = 1.0f;
    return out;
}

inline D3DXMATRIX *D3DXMatrixMultiply(D3DXMATRIX *out, const D3DXMATRIX *a, const D3DXMATRIX *b)
{
    D3DXMATRIX tmp;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            tmp.m[i][j] = 0;
            for (int k = 0; k < 4; k++)
                tmp.m[i][j] += a->m[i][k] * b->m[k][j];
        }
    *out = tmp;
    return out;
}

inline D3DXMATRIX *D3DXMatrixRotationX(D3DXMATRIX *out, f32 angle)
{
    D3DXMatrixIdentity(out);
    f32 c = cosf(angle), s = sinf(angle);
    out->_22 = c;  out->_23 = s;
    out->_32 = -s; out->_33 = c;
    return out;
}

inline D3DXMATRIX *D3DXMatrixRotationY(D3DXMATRIX *out, f32 angle)
{
    D3DXMatrixIdentity(out);
    f32 c = cosf(angle), s = sinf(angle);
    out->_11 = c;  out->_13 = -s;
    out->_31 = s;  out->_33 = c;
    return out;
}

inline D3DXMATRIX *D3DXMatrixRotationZ(D3DXMATRIX *out, f32 angle)
{
    D3DXMatrixIdentity(out);
    f32 c = cosf(angle), s = sinf(angle);
    out->_11 = c;  out->_12 = s;
    out->_21 = -s; out->_22 = c;
    return out;
}

inline D3DXMATRIX *D3DXMatrixTranslation(D3DXMATRIX *out, f32 x, f32 y, f32 z)
{
    D3DXMatrixIdentity(out);
    out->_41 = x; out->_42 = y; out->_43 = z;
    return out;
}

inline D3DXMATRIX *D3DXMatrixScaling(D3DXMATRIX *out, f32 sx, f32 sy, f32 sz)
{
    D3DXMatrixIdentity(out);
    out->_11 = sx; out->_22 = sy; out->_33 = sz;
    return out;
}

inline D3DXMATRIX *D3DXMatrixLookAtLH(D3DXMATRIX *out, const D3DXVECTOR3 *eye, const D3DXVECTOR3 *at, const D3DXVECTOR3 *up)
{
    D3DXVECTOR3 zaxis = {at->x - eye->x, at->y - eye->y, at->z - eye->z};
    f32 zlen = sqrtf(zaxis.x*zaxis.x + zaxis.y*zaxis.y + zaxis.z*zaxis.z);
    if (zlen > 0) { zaxis.x /= zlen; zaxis.y /= zlen; zaxis.z /= zlen; }

    D3DXVECTOR3 xaxis = {
        up->y * zaxis.z - up->z * zaxis.y,
        up->z * zaxis.x - up->x * zaxis.z,
        up->x * zaxis.y - up->y * zaxis.x
    };
    f32 xlen = sqrtf(xaxis.x*xaxis.x + xaxis.y*xaxis.y + xaxis.z*xaxis.z);
    if (xlen > 0) { xaxis.x /= xlen; xaxis.y /= xlen; xaxis.z /= xlen; }

    D3DXVECTOR3 yaxis = {
        zaxis.y * xaxis.z - zaxis.z * xaxis.y,
        zaxis.z * xaxis.x - zaxis.x * xaxis.z,
        zaxis.x * xaxis.y - zaxis.y * xaxis.x
    };

    D3DXMatrixIdentity(out);
    out->_11 = xaxis.x; out->_21 = xaxis.y; out->_31 = xaxis.z;
    out->_12 = yaxis.x; out->_22 = yaxis.y; out->_32 = yaxis.z;
    out->_13 = zaxis.x; out->_23 = zaxis.y; out->_33 = zaxis.z;
    out->_41 = -(xaxis.x*eye->x + xaxis.y*eye->y + xaxis.z*eye->z);
    out->_42 = -(yaxis.x*eye->x + yaxis.y*eye->y + yaxis.z*eye->z);
    out->_43 = -(zaxis.x*eye->x + zaxis.y*eye->y + zaxis.z*eye->z);
    return out;
}

inline D3DXMATRIX *D3DXMatrixPerspectiveFovLH(D3DXMATRIX *out, f32 fovy, f32 aspect, f32 zn, f32 zf)
{
    f32 yScale = 1.0f / tanf(fovy / 2.0f);
    f32 xScale = yScale / aspect;
    memset(out->m, 0, sizeof(out->m));
    out->_11 = xScale;
    out->_22 = yScale;
    out->_33 = zf / (zf - zn);
    out->_34 = 1.0f;
    out->_43 = -zn * zf / (zf - zn);
    return out;
}

inline D3DXMATRIX *D3DXMatrixOrthoOffCenterLH(D3DXMATRIX *out, f32 l, f32 r, f32 b, f32 t, f32 zn, f32 zf)
{
    memset(out->m, 0, sizeof(out->m));
    out->_11 = 2.0f / (r - l);
    out->_22 = 2.0f / (t - b);
    out->_33 = 1.0f / (zf - zn);
    out->_41 = (l + r) / (l - r);
    out->_42 = (t + b) / (b - t);
    out->_43 = zn / (zn - zf);
    out->_44 = 1.0f;
    return out;
}

#ifndef D3DX_PI
#define D3DX_PI 3.14159265358979323846f
#endif
#define D3DXToRadian(degree) ((degree) * (D3DX_PI / 180.0f))

inline f32 D3DXVec3Length(const D3DXVECTOR3 *v)
{
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

inline f32 D3DXVec3LengthSq(const D3DXVECTOR3 *v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

inline D3DXVECTOR3 *D3DXVec3Normalize(D3DXVECTOR3 *out, const D3DXVECTOR3 *v)
{
    f32 len = D3DXVec3Length(v);
    if (len > 0.0f) { out->x = v->x / len; out->y = v->y / len; out->z = v->z / len; }
    else { out->x = out->y = out->z = 0.0f; }
    return out;
}

inline D3DXMATRIX *D3DXMatrixRotationQuaternion(D3DXMATRIX *out, const D3DXQUATERNION *q)
{
    f32 xx = q->x * q->x, yy = q->y * q->y, zz = q->z * q->z;
    f32 xy = q->x * q->y, xz = q->x * q->z, yz = q->y * q->z;
    f32 wx = q->w * q->x, wy = q->w * q->y, wz = q->w * q->z;
    D3DXMatrixIdentity(out);
    out->_11 = 1.0f - 2.0f * (yy + zz);
    out->_12 = 2.0f * (xy + wz);
    out->_13 = 2.0f * (xz - wy);
    out->_21 = 2.0f * (xy - wz);
    out->_22 = 1.0f - 2.0f * (xx + zz);
    out->_23 = 2.0f * (yz + wx);
    out->_31 = 2.0f * (xz + wy);
    out->_32 = 2.0f * (yz - wx);
    out->_33 = 1.0f - 2.0f * (xx + yy);
    return out;
}

inline D3DXVECTOR3 *D3DXVec3TransformCoord(D3DXVECTOR3 *out, const D3DXVECTOR3 *v, const D3DXMATRIX *m)
{
    f32 x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31 + m->_41;
    f32 y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32 + m->_42;
    f32 z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33 + m->_43;
    f32 w = v->x * m->_14 + v->y * m->_24 + v->z * m->_34 + m->_44;
    if (w != 0.0f) { out->x = x / w; out->y = y / w; out->z = z / w; }
    else { out->x = x; out->y = y; out->z = z; }
    return out;
}

// Forward-declare D3DVIEWPORT8 so D3DXVec3Project can use it
struct D3DVIEWPORT8
{
    u32 X, Y, Width, Height;
    f32 MinZ, MaxZ;
};

inline D3DXVECTOR3 *D3DXVec3Project(D3DXVECTOR3 *out, const D3DXVECTOR3 *v,
                                     const D3DVIEWPORT8 *vp,
                                     const D3DXMATRIX *proj,
                                     const D3DXMATRIX *view,
                                     const D3DXMATRIX *world)
{
    D3DXMATRIX wv, wvp;
    D3DXMatrixMultiply(&wv, world, view);
    D3DXMatrixMultiply(&wvp, &wv, proj);
    f32 x = v->x * wvp._11 + v->y * wvp._21 + v->z * wvp._31 + wvp._41;
    f32 y = v->x * wvp._12 + v->y * wvp._22 + v->z * wvp._32 + wvp._42;
    f32 z = v->x * wvp._13 + v->y * wvp._23 + v->z * wvp._33 + wvp._43;
    f32 w = v->x * wvp._14 + v->y * wvp._24 + v->z * wvp._34 + wvp._44;
    if (w != 0.0f) { x /= w; y /= w; z /= w; }
    out->x = vp->X + (1.0f + x) * vp->Width * 0.5f;
    out->y = vp->Y + (1.0f - y) * vp->Height * 0.5f;
    out->z = vp->MinZ + z * (vp->MaxZ - vp->MinZ);
    return out;
}

// =============================================================================
// D3D present parameters shim (stored but not used for actual D3D calls)
// =============================================================================
struct D3DPRESENT_PARAMETERS
{
    u32 BackBufferWidth;
    u32 BackBufferHeight;
    u32 BackBufferFormat;
    u32 BackBufferCount;
    u32 MultiSampleType;
    u32 SwapEffect;
    void *hDeviceWindow;
    i32 Windowed;
    i32 EnableAutoDepthStencil;
    u32 AutoDepthStencilFormat;
    u32 Flags;
    u32 FullScreen_RefreshRateInHz;
    u32 FullScreen_PresentationInterval;
};

// =============================================================================
// D3D caps shim
// =============================================================================
struct D3DCAPS8
{
    u8 padding[380];
};

// =============================================================================
// D3D format shim
// =============================================================================
typedef u32 D3DFORMAT;

// D3D format constants (used by TextHelper pixel format tables)
#define D3DFMT_X8R8G8B8  22
#define D3DFMT_A8R8G8B8  21
#define D3DFMT_X1R5G5B5  24
#define D3DFMT_R5G6B5    23
#define D3DFMT_A1R5G5B5  25
#define D3DFMT_A4R4G4B4  26

// =============================================================================
// Soft surface — replaces IDirect3DSurface8 for pixel buffer operations
// =============================================================================
struct SoftSurface
{
    u8 *pixels;
    i32 width;
    i32 height;
    i32 pitch;
    D3DFORMAT format;

    void Release()
    {
        if (pixels)
        {
            free(pixels);
            pixels = NULL;
        }
        delete this;
    }
};

struct D3DSURFACE_DESC
{
    D3DFORMAT Format;
    u32 Width;
    u32 Height;
};

struct D3DLOCKED_RECT
{
    i32 Pitch;
    void *pBits;
};

// =============================================================================
// D3D image info shim
// =============================================================================
struct D3DXIMAGE_INFO
{
    u32 Width;
    u32 Height;
    u32 Depth;
    u32 MipLevels;
    D3DFORMAT Format;
    u32 ResourceType;
    u32 ImageFileFormat;
};

// =============================================================================
// Win32 type shims (for code that still references them)
// =============================================================================
#ifndef _WINDOWS_
typedef void *HWND;
typedef void *HINSTANCE;
typedef void *HANDLE;
typedef unsigned long DWORD;
typedef DWORD *LPDWORD;
typedef int BOOL;
typedef unsigned int UINT;
typedef long LONG;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef long LRESULT;
typedef unsigned long WPARAM;
typedef long LPARAM;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef void *LPVOID;
typedef unsigned long DWORD_PTR;
typedef unsigned long *LPDWORD;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

struct FILETIME { DWORD dwLowDateTime; DWORD dwHighDateTime; };
typedef FILETIME *LPFILETIME;

struct RECT { LONG left, top, right, bottom; };
struct POINT { LONG x, y; };

typedef struct {
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    BYTE Data4[8];
} GUID;

typedef struct {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX;

#endif // _WINDOWS_

// WAVEFORMATEX 可能在 WIN32_LEAN_AND_MEAN 模式下不被 windows.h 包含
#if defined(_WINDOWS_) && !defined(_WAVEFORMATEX_)
#include <mmsystem.h>
#endif

// byte type (provided by rpcndr.h on Windows, may be missing with LEAN_AND_MEAN)
#ifndef _RPCNDR_H
#ifndef __RPCNDR_H__
typedef unsigned char byte;
#endif
#endif

// =============================================================================
// Timing
// =============================================================================
#ifndef _WINDOWS_
inline DWORD timeGetTime()
{
    return (DWORD)SDL_GetTicks();
}
#endif

// =============================================================================
// C_ASSERT macro (provided by winnt.h on Windows, need our own for other platforms)
// =============================================================================
#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif

// =============================================================================
// SAFE_DELETE / SAFE_RELEASE macros (from dxutil.hpp, simplified)
// =============================================================================
#ifndef SAFE_DELETE
#define SAFE_DELETE(p) { if(p) { delete (p); (p) = NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p) = NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p) = NULL; } }
#endif
