// =============================================================================
// sdl2_renderer.cpp - OpenGL rendering backend implementation for th06
// =============================================================================

#include "sdl2_renderer.hpp"
#include "AnmManager.hpp"
#include <SDL_image.h>
#include <cstring>
#include <vector>

namespace th06
{

static GLboolean g_PrevDepthTestEnabled = GL_FALSE;
static GLboolean g_PrevDepthMask = GL_TRUE;
static GLint g_PrevScissorBox[4] = {0, 0, 640, 480};

static void UploadRgbaSurface(GLuint tex, SDL_Surface *rgba)
{
    const i32 bytesPerPixel = 4;
    const i32 tightPitch = rgba->w * bytesPerPixel;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (rgba->pitch == tightPitch)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
        return;
    }

    // SDL surfaces may have padded rows; repack to tightly packed RGBA rows for GL upload.
    std::vector<u8> packed((size_t)tightPitch * (size_t)rgba->h);
    const u8 *src = (const u8 *)rgba->pixels;
    for (i32 y = 0; y < rgba->h; ++y)
    {
        memcpy(&packed[(size_t)y * (size_t)tightPitch], src + (size_t)y * (size_t)rgba->pitch, (size_t)tightPitch);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, packed.data());
}

static void ApplySamplerFor2D()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static void ApplySamplerFor3D()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

SDL2Renderer g_Renderer;

void SDL2Renderer::Init(SDL_Window *win, SDL_GLContext ctx, i32 w, i32 h)
{
    this->window = win;
    this->glContext = ctx;
    this->screenWidth = w;
    this->screenHeight = h;
    this->currentTexture = 0;
    this->currentBlendMode = 0xff;
    this->currentColorOp = 0xff;
    this->currentVertexShader = 0xff;
    this->currentZWriteDisable = 0xff;
    this->textureFactor = 0xffffffff;
    this->fogEnabled = 0;
    this->fogColor = 0xffa0a0a0;
    this->fogStart = 1000.0f;
    this->fogEnd = 5000.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 4.0f / 255.0f);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_SCISSOR_TEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // Match D3D's [0,1] depth range (wined3d does the same).
    glDepthRange(0.0, 1.0);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    SetViewport(0, 0, w, h);
}

void SDL2Renderer::BeginScene()
{
    // OpenGL doesn't have explicit begin/end scene
}

void SDL2Renderer::EndScene()
{
    // OpenGL doesn't have explicit begin/end scene
}

void SDL2Renderer::Clear(D3DCOLOR color, i32 clearColor, i32 clearDepth)
{
    GLbitfield mask = 0;
    if (clearColor)
    {
        f32 a = D3DCOLOR_A(color) / 255.0f;
        f32 r = D3DCOLOR_R(color) / 255.0f;
        f32 g = D3DCOLOR_G(color) / 255.0f;
        f32 b = D3DCOLOR_B(color) / 255.0f;
        glClearColor(r, g, b, a);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (clearDepth)
        mask |= GL_DEPTH_BUFFER_BIT;
    if (mask)
        glClear(mask);
}

void SDL2Renderer::SetViewport(i32 x, i32 y, i32 w, i32 h)
{
    this->viewportX = x;
    this->viewportY = y;
    this->viewportW = w;
    this->viewportH = h;
    glViewport(x, this->screenHeight - y - h, w, h);
    glScissor(x, this->screenHeight - y - h, w, h);
}

void SDL2Renderer::SetBlendMode(u8 mode)
{
    if (mode == this->currentBlendMode)
        return;
    this->currentBlendMode = mode;
    if (mode == BLEND_MODE_ADD)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    else
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void SDL2Renderer::SetTexture(GLuint tex)
{
    if (tex == this->currentTexture)
        return;
    this->currentTexture = tex;
    if (tex != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void SDL2Renderer::SetTextureFactor(D3DCOLOR factor)
{
    this->textureFactor = factor;
}

void SDL2Renderer::SetZWriteDisable(u8 disable)
{
    if (disable == this->currentZWriteDisable)
        return;
    this->currentZWriteDisable = disable;
    glDepthMask(disable ? GL_FALSE : GL_TRUE);
}

void SDL2Renderer::SetDepthFunc(i32 alwaysPass)
{
    glDepthFunc(alwaysPass ? GL_ALWAYS : GL_LEQUAL);
}

void SDL2Renderer::SetDestBlendInvSrcAlpha()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SDL2Renderer::SetDestBlendOne()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void SDL2Renderer::SetTextureStageSelectDiffuse()
{
    glDisable(GL_TEXTURE_2D);
    this->currentTexture = 0;
}

void SDL2Renderer::SetTextureStageModulateTexture()
{
    glEnable(GL_TEXTURE_2D);
}

void SDL2Renderer::SetFog(i32 enable, D3DCOLOR color, f32 start, f32 end)
{
    this->fogEnabled = enable;
    this->fogColor = color;
    this->fogStart = start;
    this->fogEnd = end;
    if (enable)
    {
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        f32 fogCol[4] = {
            D3DCOLOR_R(color) / 255.0f,
            D3DCOLOR_G(color) / 255.0f,
            D3DCOLOR_B(color) / 255.0f,
            D3DCOLOR_A(color) / 255.0f
        };
        glFogfv(GL_FOG_COLOR, fogCol);
        glFogf(GL_FOG_START, start);
        glFogf(GL_FOG_END, end);
    }
    else
    {
        glDisable(GL_FOG);
    }
}

static void ApplyVertexColor(D3DCOLOR c)
{
    glColor4ub(D3DCOLOR_R(c), D3DCOLOR_G(c), D3DCOLOR_B(c), D3DCOLOR_A(c));
}

static void Begin2DDraw(SDL2Renderer *r)
{
    g_PrevDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &g_PrevDepthMask);
    glGetIntegerv(GL_SCISSOR_BOX, g_PrevScissorBox);

    // Pre-transformed (XYZRHW) vertices are in full screen-space coordinates.
    // Set both viewport AND scissor to full screen to match D3D's behavior
    // where XYZRHW vertices bypass viewport transform. The D3D viewport clip
    // was already applied via SetViewport's scissor rect; here we need the
    // full screen to position vertices correctly in the ortho projection.
    glViewport(0, 0, r->screenWidth, r->screenHeight);
    glScissor(0, 0, r->screenWidth, r->screenHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // 2D sprite path should not inherit texture transforms from 3D/effect passes.
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // Map D3D transformed z in [0,1] to OpenGL depth the same way (0 near, 1 far).
    glOrtho(0, r->screenWidth, r->screenHeight, 0, 0, -1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (r->currentTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, r->currentTexture);
        ApplySamplerFor2D();
    }
}

static void End2DDraw(SDL2Renderer *r)
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    // Restore viewport and scissor to the values set by SetViewport.
    glViewport(r->viewportX, r->screenHeight - r->viewportY - r->viewportH, r->viewportW, r->viewportH);
    glScissor(g_PrevScissorBox[0], g_PrevScissorBox[1], g_PrevScissorBox[2], g_PrevScissorBox[3]);

    if (g_PrevDepthTestEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(g_PrevDepthMask);
}

void SDL2Renderer::DrawTriangleStrip(const VertexDiffuseXyzrwh *verts, i32 count)
{
    glDisable(GL_TEXTURE_2D);
    Begin2DDraw(this);

    glBegin(GL_TRIANGLE_STRIP);
    for (i32 i = 0; i < count; i++)
    {
        ApplyVertexColor(verts[i].diffuse);
        // XYZRHW vertices are already in screen space — use (x, y, z) directly.
        // The w component (1/W) is for perspective-correct interpolation only;
        // wined3d does not divide positions by w for pre-transformed vertices.
        glVertex3f(verts[i].position.x, verts[i].position.y, verts[i].position.z);
    }
    glEnd();

    End2DDraw(this);
    if (this->currentTexture != 0)
        glEnable(GL_TEXTURE_2D);
}

void SDL2Renderer::DrawTriangleStripTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count)
{
    Begin2DDraw(this);

    glBegin(GL_TRIANGLE_STRIP);
    for (i32 i = 0; i < count; i++)
    {
        ApplyVertexColor(verts[i].diffuse);
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        f32 x = verts[i].position.x;
        f32 y = verts[i].position.y;
        f32 z = verts[i].position.z;
        glVertex3f(x, y, z);
    }
    glEnd();

    End2DDraw(this);
}

void SDL2Renderer::DrawTriangleStripTextured3D(const VertexTex1DiffuseXyz *verts, i32 count)
{
    // Apply view/projection matrices
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(&this->projectionMatrix.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(&this->viewMatrix.m[0][0]);

    glBegin(GL_TRIANGLE_STRIP);
    if (this->currentTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
        ApplySamplerFor3D();
    }
    for (i32 i = 0; i < count; i++)
    {
        ApplyVertexColor(verts[i].diffuse);
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        glVertex3f(verts[i].position.x, verts[i].position.y, verts[i].position.z);
    }
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void SDL2Renderer::DrawTriangleFanTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count)
{
    Begin2DDraw(this);

    glBegin(GL_TRIANGLE_FAN);
    for (i32 i = 0; i < count; i++)
    {
        ApplyVertexColor(verts[i].diffuse);
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        f32 x = verts[i].position.x;
        f32 y = verts[i].position.y;
        f32 z = verts[i].position.z;
        glVertex3f(x, y, z);
    }
    glEnd();

    End2DDraw(this);
}

void SDL2Renderer::DrawTriangleFanTextured3D(const VertexTex1DiffuseXyz *verts, i32 count)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(&this->projectionMatrix.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(&this->viewMatrix.m[0][0]);

    glBegin(GL_TRIANGLE_FAN);
    if (this->currentTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
        ApplySamplerFor3D();
    }
    for (i32 i = 0; i < count; i++)
    {
        ApplyVertexColor(verts[i].diffuse);
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        glVertex3f(verts[i].position.x, verts[i].position.y, verts[i].position.z);
    }
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

GLuint SDL2Renderer::CreateTextureFromMemory(const u8 *data, i32 dataLen, D3DCOLOR colorKey, i32 *outWidth, i32 *outHeight)
{
    SDL_RWops *rw = SDL_RWFromConstMem(data, dataLen);
    if (!rw) return 0;

    SDL_Surface *surface = IMG_Load_RW(rw, 1);
    if (!surface) return 0;

    // Convert to RGBA
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return 0;

    // Apply color key transparency
    if (colorKey != 0)
    {
        u8 ckR = D3DCOLOR_R(colorKey);
        u8 ckG = D3DCOLOR_G(colorKey);
        u8 ckB = D3DCOLOR_B(colorKey);
        SDL_LockSurface(rgba);
        u8 *pixels = (u8 *)rgba->pixels;
        i32 pixelCount = rgba->w * rgba->h;
        for (i32 i = 0; i < pixelCount; i++)
        {
            u8 *p = pixels + i * 4;
            if (p[0] == ckR && p[1] == ckG && p[2] == ckB)
            {
                p[3] = 0;
            }
        }
        SDL_UnlockSurface(rgba);
    }

    if (outWidth) *outWidth = rgba->w;
    if (outHeight) *outHeight = rgba->h;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    UploadRgbaSurface(tex, rgba);

    SDL_FreeSurface(rgba);

    // Restore current texture binding
    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    return tex;
}

GLuint SDL2Renderer::CreateEmptyTexture(i32 width, i32 height)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    return tex;
}

void SDL2Renderer::DeleteTexture(GLuint tex)
{
    if (tex != 0)
    {
        if (this->currentTexture == tex)
        {
            this->currentTexture = 0;
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glDeleteTextures(1, &tex);
    }
}

void SDL2Renderer::SetColorOp(u8 colorOp)
{
    if (colorOp == this->currentColorOp)
        return;
    this->currentColorOp = colorOp;
    if (colorOp == 0) // Modulate
    {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    else // Add
    {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
    }
}

void SDL2Renderer::SetViewTransform(const D3DXMATRIX *matrix)
{
    this->viewMatrix = *matrix;
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&this->viewMatrix.m[0][0]);
}

void SDL2Renderer::SetProjectionTransform(const D3DXMATRIX *matrix)
{
    this->projectionMatrix = *matrix;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&this->projectionMatrix.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
}

void SDL2Renderer::SetWorldTransform(const D3DXMATRIX *matrix)
{
    glMatrixMode(GL_MODELVIEW);
    D3DXMATRIX combined;
    D3DXMatrixMultiply(&combined, matrix, &this->viewMatrix);
    glLoadMatrixf(&combined.m[0][0]);
}

void SDL2Renderer::SetTextureTransform(const D3DXMATRIX *matrix)
{
    // D3D uses row-vector * matrix (row-major), GL uses matrix * column-vector (column-major).
    // glLoadMatrixf reads the row-major D3D data as column-major, which naturally transposes
    // the matrix — this is exactly the transpose needed for the convention change.
    // (Same approach as wined3d: pass D3D matrices directly to GL.)
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(&matrix->m[0][0]);
}

GLuint SDL2Renderer::LoadSurfaceFromFile(const u8 *data, i32 dataLen, i32 *outWidth, i32 *outHeight)
{
    SDL_RWops *rw = SDL_RWFromConstMem(data, dataLen);
    if (!rw) return 0;
    SDL_Surface *surface = IMG_Load_RW(rw, 1);
    if (!surface) return 0;

    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return 0;

    if (outWidth) *outWidth = rgba->w;
    if (outHeight) *outHeight = rgba->h;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    UploadRgbaSurface(tex, rgba);

    SDL_FreeSurface(rgba);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    return tex;
}

void SDL2Renderer::CopySurfaceToScreen(GLuint surfaceTex, i32 srcX, i32 srcY, i32 dstX, i32 dstY, i32 w, i32 h,
                                       i32 texW, i32 texH)
{
    if (surfaceTex == 0) return;

    f32 u0 = (f32)srcX / texW;
    f32 v0 = (f32)srcY / texH;
    f32 u1 = (f32)(srcX + (w > 0 ? w : texW)) / texW;
    f32 v1 = (f32)(srcY + (h > 0 ? h : texH)) / texH;

    i32 drawW = w > 0 ? w : texW;
    i32 drawH = h > 0 ? h : texH;

    Begin2DDraw(this);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, surfaceTex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(u0, v0); glVertex2f((f32)dstX, (f32)dstY);
    glTexCoord2f(u1, v0); glVertex2f((f32)(dstX + drawW), (f32)dstY);
    glTexCoord2f(u0, v1); glVertex2f((f32)dstX, (f32)(dstY + drawH));
    glTexCoord2f(u1, v1); glVertex2f((f32)(dstX + drawW), (f32)(dstY + drawH));
    glEnd();

    End2DDraw(this);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
}

void SDL2Renderer::CopySurfaceRectToScreen(GLuint surfaceTex, i32 srcX, i32 srcY, i32 srcW, i32 srcH,
                                           i32 dstX, i32 dstY, i32 texW, i32 texH)
{
    if (surfaceTex == 0) return;

    f32 u0 = (f32)srcX / texW;
    f32 v0 = (f32)srcY / texH;
    f32 u1 = (f32)(srcX + srcW) / texW;
    f32 v1 = (f32)(srcY + srcH) / texH;

    Begin2DDraw(this);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, surfaceTex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(u0, v0); glVertex2f((f32)dstX, (f32)dstY);
    glTexCoord2f(u1, v0); glVertex2f((f32)(dstX + srcW), (f32)dstY);
    glTexCoord2f(u0, v1); glVertex2f((f32)dstX, (f32)(dstY + srcH));
    glTexCoord2f(u1, v1); glVertex2f((f32)(dstX + srcW), (f32)(dstY + srcH));
    glEnd();

    End2DDraw(this);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
}

void SDL2Renderer::TakeScreenshot(GLuint dstTex, i32 left, i32 top, i32 width, i32 height)
{
    if (dstTex == 0) return;

    u8 *pixels = new u8[width * height * 4];
    glReadPixels(left, this->screenHeight - top - height, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Flip vertically (OpenGL reads bottom-up)
    u8 *flipped = new u8[width * height * 4];
    for (i32 y = 0; y < height; y++)
    {
        memcpy(flipped + y * width * 4, pixels + (height - 1 - y) * width * 4, width * 4);
    }

    glBindTexture(GL_TEXTURE_2D, dstTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, flipped);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    delete[] pixels;
    delete[] flipped;
}

void SDL2Renderer::CopyAlphaChannel(GLuint dstTex, const u8 *srcData, i32 dataLen, i32 width, i32 height)
{
    if (dstTex == 0) return;

    SDL_RWops *rw = SDL_RWFromConstMem(srcData, dataLen);
    if (!rw) return;
    SDL_Surface *srcSurface = IMG_Load_RW(rw, 1);
    if (!srcSurface) return;

    SDL_Surface *srcRgba = SDL_ConvertSurfaceFormat(srcSurface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(srcSurface);
    if (!srcRgba) return;

    glBindTexture(GL_TEXTURE_2D, dstTex);

    // Query actual texture dimensions if caller passed 0
    if (width <= 0 || height <= 0)
    {
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    }
    if (width <= 0 || height <= 0)
    {
        SDL_FreeSurface(srcRgba);
        return;
    }

    // Read current texture pixels
    u8 *dstPixels = new u8[width * height * 4];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dstPixels);

    // Copy blue channel of source as alpha channel of destination
    i32 copyW = srcRgba->w < width ? srcRgba->w : width;
    i32 copyH = srcRgba->h < height ? srcRgba->h : height;
    SDL_LockSurface(srcRgba);
    for (i32 y = 0; y < copyH; y++)
    {
        u8 *src = (u8 *)srcRgba->pixels + y * srcRgba->pitch;
        u8 *dst = dstPixels + y * width * 4;
        for (i32 x = 0; x < copyW; x++)
        {
            dst[x * 4 + 3] = src[x * 4 + 0]; // blue → alpha
        }
    }
    SDL_UnlockSurface(srcRgba);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dstPixels);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    delete[] dstPixels;
    SDL_FreeSurface(srcRgba);
}

void SDL2Renderer::UpdateTextureSubImage(GLuint tex, i32 x, i32 y, i32 w, i32 h, const u8 *rgbaPixels)
{
    if (tex == 0 || !rgbaPixels) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);
    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
}

void SDL2Renderer::DrawTriangleStripTex(const VertexTex1Xyzrwh *verts, i32 count)
{
    Begin2DDraw(this);

    // Use texture factor as vertex color
    ApplyVertexColor(this->textureFactor);

    glBegin(GL_TRIANGLE_STRIP);
    for (i32 i = 0; i < count; i++)
    {
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        f32 x = verts[i].position.x;
        f32 y = verts[i].position.y;
        f32 z = verts[i].position.z;
        glVertex3f(x, y, z);
    }
    glEnd();

    End2DDraw(this);
}

void SDL2Renderer::DrawVertexBuffer3D(const RenderVertexInfo *verts, i32 count)
{
    // Uses currently set world transform and texture transform
    // Projection already setup via glMatrixMode calls

    ApplyVertexColor(this->textureFactor);

    if (this->currentTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
        ApplySamplerFor3D();
    }

    glBegin(GL_TRIANGLE_STRIP);
    for (i32 i = 0; i < count; i++)
    {
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        glVertex3f(verts[i].position.x, verts[i].position.y, verts[i].position.z);
    }
    glEnd();
}

GLuint SDL2Renderer::LoadSurfaceFromFile(const u8 *data, i32 dataLen, D3DXIMAGE_INFO *info)
{
    i32 w = 0, h = 0;
    GLuint result = LoadSurfaceFromFile(data, dataLen, &w, &h);
    if (info)
    {
        info->Width = (u32)w;
        info->Height = (u32)h;
    }
    return result;
}

void SDL2Renderer::CopySurfaceToScreen(GLuint surfaceTex, i32 texW, i32 texH, i32 dstX, i32 dstY)
{
    CopySurfaceToScreen(surfaceTex, 0, 0, dstX, dstY, texW, texH, texW, texH);
}

} // namespace th06
