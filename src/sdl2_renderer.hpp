#pragma once
// =============================================================================
// sdl2_renderer.hpp - OpenGL rendering backend for th06 (replaces D3D8)
// =============================================================================

#include "sdl2_compat.hpp"
#include <SDL.h>
#include <SDL_opengl.h>

namespace th06
{

// Blend modes matching original D3D8 blend states
enum BlendMode
{
    BLEND_MODE_ALPHA = 0,         // SrcAlpha, InvSrcAlpha
    BLEND_MODE_ADD = 1,           // SrcAlpha, One
    BLEND_MODE_INV_SRC_ALPHA = 0, // alias for ALPHA
    BLEND_MODE_ADDITIVE = 1,      // alias for ADD
};

// Vertex structures matching original FVF formats
struct VertexDiffuseXyzrwh
{
    D3DXVECTOR4 position;
    D3DCOLOR diffuse;
};

struct VertexTex1Xyzrwh
{
    D3DXVECTOR4 position;
    D3DXVECTOR2 textureUV;
};

struct VertexTex1DiffuseXyzrwh
{
    D3DXVECTOR4 position;
    D3DCOLOR diffuse;
    D3DXVECTOR2 textureUV;
};

struct VertexTex1DiffuseXyz
{
    D3DXVECTOR3 position;
    D3DCOLOR diffuse;
    D3DXVECTOR2 textureUV;
};

// RenderVertexInfo is defined in AnmManager.hpp
struct RenderVertexInfo;

// Global renderer state
struct SDL2Renderer
{
    SDL_Window *window;
    SDL_GLContext glContext;
    i32 screenWidth;
    i32 screenHeight;
    i32 viewportX, viewportY, viewportW, viewportH;

    // Current render state
    GLuint currentTexture;
    u8 currentBlendMode;
    u8 currentColorOp;
    u8 currentVertexShader;
    u8 currentZWriteDisable;
    D3DCOLOR textureFactor;

    // Projection/view matrices for 3D mode
    D3DXMATRIX viewMatrix;
    D3DXMATRIX projectionMatrix;

    i32 fogEnabled;
    D3DCOLOR fogColor;
    f32 fogStart;
    f32 fogEnd;

    void Init(SDL_Window *win, SDL_GLContext ctx, i32 w, i32 h);
    void BeginScene();
    void EndScene();
    void Clear(D3DCOLOR color, i32 clearColor, i32 clearDepth);
    void SetViewport(i32 x, i32 y, i32 w, i32 h);
    void SetBlendMode(u8 mode);
    void SetColorOp(u8 colorOp);
    void SetTexture(GLuint tex);
    void SetTextureFactor(D3DCOLOR factor);
    void SetZWriteDisable(u8 disable);
    void SetDepthFunc(i32 alwaysPass);
    void SetDestBlendInvSrcAlpha();
    void SetDestBlendOne();
    void SetTextureStageSelectDiffuse();
    void SetTextureStageModulateTexture();
    void SetFog(i32 enable, D3DCOLOR color, f32 start, f32 end);
    void SetViewTransform(const D3DXMATRIX *matrix);
    void SetProjectionTransform(const D3DXMATRIX *matrix);
    void SetWorldTransform(const D3DXMATRIX *matrix);
    void SetTextureTransform(const D3DXMATRIX *matrix);

    // Draw primitives (match D3D8 DrawPrimitiveUP)
    void DrawTriangleStrip(const VertexDiffuseXyzrwh *verts, i32 count);
    void DrawTriangleStripTex(const VertexTex1Xyzrwh *verts, i32 count);
    void DrawTriangleStripTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count);
    void DrawTriangleStripTextured3D(const VertexTex1DiffuseXyz *verts, i32 count);
    void DrawTriangleFanTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count);
    void DrawTriangleFanTextured3D(const VertexTex1DiffuseXyz *verts, i32 count);
    void DrawVertexBuffer3D(const RenderVertexInfo *verts, i32 count);

    // Texture management
    GLuint CreateTextureFromMemory(const u8 *data, i32 dataLen, D3DCOLOR colorKey, i32 *outWidth, i32 *outHeight);
    GLuint CreateEmptyTexture(i32 width, i32 height);
    void DeleteTexture(GLuint tex);
    void CopyAlphaChannel(GLuint dstTex, const u8 *srcData, i32 dataLen, i32 width, i32 height);
    void UpdateTextureSubImage(GLuint tex, i32 x, i32 y, i32 w, i32 h, const u8 *rgbaPixels);

    // Surface operations (for screenshots, image loading)
    GLuint LoadSurfaceFromFile(const u8 *data, i32 dataLen, i32 *outWidth, i32 *outHeight);
    GLuint LoadSurfaceFromFile(const u8 *data, i32 dataLen, D3DXIMAGE_INFO *info);
    void CopySurfaceToScreen(GLuint surfaceTex, i32 srcX, i32 srcY, i32 dstX, i32 dstY, i32 w, i32 h,
                             i32 texW, i32 texH);
    void CopySurfaceToScreen(GLuint surfaceTex, i32 texW, i32 texH, i32 dstX, i32 dstY);
    void CopySurfaceRectToScreen(GLuint surfaceTex, i32 srcX, i32 srcY, i32 srcW, i32 srcH,
                                 i32 dstX, i32 dstY, i32 texW, i32 texH);
    void TakeScreenshot(GLuint dstTex, i32 left, i32 top, i32 width, i32 height);
};

extern SDL2Renderer g_Renderer;

} // namespace th06
