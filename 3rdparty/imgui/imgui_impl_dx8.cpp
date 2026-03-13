// Adapted from thprac_gui_impl_dx8.cpp for native th06 integration.
// Original: thprac/thprac/src/thprac/thprac_gui_impl_dx8.cpp
// Changes:
//   1. d3d8.h include changed from local thprac path to system <d3d8.h>
//   2. Functions renamed from THPrac::Gui::ImplDX8* to ImGui_ImplDX8_*
//   3. Removed HookReset / UnHookReset (not needed in source build)
#pragma warning(disable : 4100)

#define CINTERFACE
#include "imgui_impl_dx8.h"
#include <imgui.h>
#include <d3d8.h>

struct CUSTOMVERTEX
{
    float    pos[3];
    D3DCOLOR col;
    float    uv[2];
};
static constexpr DWORD D3DFVF_CUSTOMVERTEX = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

static LPDIRECT3DDEVICE8       g_pd3dDevice = NULL;
static LPDIRECT3DVERTEXBUFFER8 g_pVB = NULL;
static LPDIRECT3DINDEXBUFFER8  g_pIB = NULL;
static LPDIRECT3DTEXTURE8      g_FontTexture = NULL;
static int                     g_VertexBufferSize = 5000, g_IndexBufferSize = 10000;

static DWORD      g_pOrigStateBlock = NULL;
static D3DMATRIX  g_pOrigWorld;
static D3DMATRIX  g_pOrigView;
static D3DMATRIX  g_pOrigProj;
static D3DVIEWPORT8              g_pOrigViewPort;
static IDirect3DBaseTexture8*   g_pOrigTexture = NULL;

static bool ImplDX8_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

    g_FontTexture = NULL;
    if (g_pd3dDevice->lpVtbl->CreateTexture(g_pd3dDevice, width, height, 1, D3DUSAGE_DYNAMIC,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_FontTexture) < 0)
        return false;

    D3DLOCKED_RECT tex_locked_rect;
    if (g_FontTexture->lpVtbl->LockRect(g_FontTexture, 0, &tex_locked_rect, NULL, 0) != D3D_OK)
        return false;
    for (int y = 0; y < height; y++)
        memcpy((unsigned char*)tex_locked_rect.pBits + tex_locked_rect.Pitch * y,
               pixels + (width * bytes_per_pixel) * y, (width * bytes_per_pixel));
    g_FontTexture->lpVtbl->UnlockRect(g_FontTexture, 0);

    io.Fonts->TexID = (ImTextureID)g_FontTexture;
    return true;
}

static void ImplDX8_SetupRenderState(ImDrawData* draw_data)
{
    g_pd3dDevice->lpVtbl->SetPixelShader(g_pd3dDevice, NULL);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_LIGHTING, FALSE);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_ZENABLE, FALSE);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_ALPHATESTENABLE, FALSE);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_BLENDOP, D3DBLENDOP_ADD);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->lpVtbl->SetTextureStageState(g_pd3dDevice, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    g_pd3dDevice->lpVtbl->SetTextureStageState(g_pd3dDevice, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_pd3dDevice->lpVtbl->SetTextureStageState(g_pd3dDevice, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_pd3dDevice->lpVtbl->SetTextureStageState(g_pd3dDevice, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g_pd3dDevice->lpVtbl->SetTextureStageState(g_pd3dDevice, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_pd3dDevice->lpVtbl->SetTextureStageState(g_pd3dDevice, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_pd3dDevice->lpVtbl->SetTextureStageState(g_pd3dDevice, 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->lpVtbl->SetTextureStageState(g_pd3dDevice, 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    g_pd3dDevice->lpVtbl->SetRenderState(g_pd3dDevice, D3DRS_FOGENABLE, FALSE);
    g_pd3dDevice->lpVtbl->SetTextureStageState(g_pd3dDevice, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

    D3DMATRIX mat_identity = { { { 1.0f, 0.0f, 0.0f, 0.0f,
                                   0.0f, 1.0f, 0.0f, 0.0f,
                                   0.0f, 0.0f, 1.0f, 0.0f,
                                   0.0f, 0.0f, 0.0f, 1.0f } } };
    g_pd3dDevice->lpVtbl->SetTransform(g_pd3dDevice, D3DTS_WORLD, &mat_identity);
    g_pd3dDevice->lpVtbl->SetTransform(g_pd3dDevice, D3DTS_VIEW, &mat_identity);
}

static bool ImplDX8_StateBackup()
{
    if (g_pd3dDevice->lpVtbl->CreateStateBlock(g_pd3dDevice, D3DSBT_ALL, &g_pOrigStateBlock) < 0)
        return false;
    g_pd3dDevice->lpVtbl->GetTransform(g_pd3dDevice, D3DTS_WORLD, &g_pOrigWorld);
    g_pd3dDevice->lpVtbl->GetTransform(g_pd3dDevice, D3DTS_VIEW, &g_pOrigView);
    g_pd3dDevice->lpVtbl->GetTransform(g_pd3dDevice, D3DTS_PROJECTION, &g_pOrigProj);
    if (g_pd3dDevice->lpVtbl->GetTexture(g_pd3dDevice, 0, &g_pOrigTexture) != D3D_OK)
        g_pOrigTexture = NULL;
    g_pd3dDevice->lpVtbl->GetViewport(g_pd3dDevice, &g_pOrigViewPort);
    return true;
}

static void ImplDX8_StateRestore()
{
    g_pd3dDevice->lpVtbl->SetTransform(g_pd3dDevice, D3DTS_WORLD, &g_pOrigWorld);
    g_pd3dDevice->lpVtbl->SetTransform(g_pd3dDevice, D3DTS_VIEW, &g_pOrigView);
    g_pd3dDevice->lpVtbl->SetTransform(g_pd3dDevice, D3DTS_PROJECTION, &g_pOrigProj);
    g_pd3dDevice->lpVtbl->ApplyStateBlock(g_pd3dDevice, g_pOrigStateBlock);
    g_pd3dDevice->lpVtbl->DeleteStateBlock(g_pd3dDevice, g_pOrigStateBlock);
    if (g_pOrigTexture) {
        g_pd3dDevice->lpVtbl->SetTexture(g_pd3dDevice, 0, g_pOrigTexture);
        g_pOrigTexture->lpVtbl->Release(g_pOrigTexture);
        g_pOrigTexture = NULL;
    }
    g_pd3dDevice->lpVtbl->SetViewport(g_pd3dDevice, &g_pOrigViewPort);
    g_pOrigStateBlock = NULL;
}

bool ImGui_ImplDX8_Init(IDirect3DDevice8* device)
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_dx8";
    g_pd3dDevice = device;
    g_pd3dDevice->lpVtbl->AddRef(g_pd3dDevice);
    return true;
}

void ImGui_ImplDX8_Shutdown()
{
    ImGui_ImplDX8_InvalidateDeviceObjects();
    if (g_pd3dDevice) {
        g_pd3dDevice->lpVtbl->Release(g_pd3dDevice);
        g_pd3dDevice = NULL;
    }
}

void ImGui_ImplDX8_NewFrame()
{
    if (!g_FontTexture)
        ImGui_ImplDX8_CreateDeviceObjects();
}

void ImGui_ImplDX8_RenderDrawData(ImDrawData* draw_data)
{
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    if (!g_pVB || g_VertexBufferSize < draw_data->TotalVtxCount) {
        if (g_pVB) { g_pVB->lpVtbl->Release(g_pVB); g_pVB = NULL; }
        g_VertexBufferSize = draw_data->TotalVtxCount + 5000;
        if (g_pd3dDevice->lpVtbl->CreateVertexBuffer(g_pd3dDevice,
                g_VertexBufferSize * sizeof(CUSTOMVERTEX),
                D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVB) < 0)
            return;
    }
    if (!g_pIB || g_IndexBufferSize < draw_data->TotalIdxCount) {
        if (g_pIB) { g_pIB->lpVtbl->Release(g_pIB); g_pIB = NULL; }
        g_IndexBufferSize = draw_data->TotalIdxCount + 10000;
        if (g_pd3dDevice->lpVtbl->CreateIndexBuffer(g_pd3dDevice,
                g_IndexBufferSize * sizeof(ImDrawIdx),
                D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32,
                D3DPOOL_DEFAULT, &g_pIB) < 0)
            return;
    }

    if (!ImplDX8_StateBackup()) return;

    CUSTOMVERTEX* vtx_dst;
    ImDrawIdx* idx_dst;
    if (g_pVB->lpVtbl->Lock(g_pVB, 0, (UINT)(draw_data->TotalVtxCount * sizeof(CUSTOMVERTEX)),
            (BYTE**)&vtx_dst, D3DLOCK_DISCARD) < 0)
        return;
    if (g_pIB->lpVtbl->Lock(g_pIB, 0, (UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)),
            (BYTE**)&idx_dst, D3DLOCK_DISCARD) < 0)
        return;

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
        for (int i = 0; i < cmd_list->VtxBuffer.Size; i++) {
            vtx_dst->pos[0] = vtx_src->pos.x;
            vtx_dst->pos[1] = vtx_src->pos.y;
            vtx_dst->pos[2] = 0.0f;
            vtx_dst->col    = vtx_src->col;
            vtx_dst->uv[0]  = vtx_src->uv.x;
            vtx_dst->uv[1]  = vtx_src->uv.y;
            vtx_dst++;
            vtx_src++;
        }
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    g_pVB->lpVtbl->Unlock(g_pVB);
    g_pIB->lpVtbl->Unlock(g_pIB);
    g_pd3dDevice->lpVtbl->SetStreamSource(g_pd3dDevice, 0, g_pVB, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->lpVtbl->SetVertexShader(g_pd3dDevice, D3DFVF_CUSTOMVERTEX);

    ImplDX8_SetupRenderState(draw_data);

    int vtx_offset = 0;
    int idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL) {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImplDX8_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            } else {
                D3DVIEWPORT8 viewport;
                viewport.X      = (DWORD)((pcmd->ClipRect.x > 0.0f) ? pcmd->ClipRect.x : 0.0f);
                viewport.Y      = (DWORD)((pcmd->ClipRect.y > 0.0f) ? pcmd->ClipRect.y : 0.0f);
                viewport.Width  = (DWORD)(draw_data->DisplaySize.x < pcmd->ClipRect.z - pcmd->ClipRect.x
                                        ? draw_data->DisplaySize.x : pcmd->ClipRect.z - pcmd->ClipRect.x);
                viewport.Height = (DWORD)(draw_data->DisplaySize.y < pcmd->ClipRect.w - pcmd->ClipRect.y
                                        ? draw_data->DisplaySize.y : pcmd->ClipRect.w - pcmd->ClipRect.y);
                viewport.MinZ   = 0.0f;
                viewport.MaxZ   = 1.0f;

                const float L = 0.5f + viewport.X, R = viewport.Width  + 0.5f + viewport.X;
                const float T = 0.5f + viewport.Y, B = viewport.Height + 0.5f + viewport.Y;
                D3DMATRIX matProj = {
                    2.0f/(R-L),       0.0f,       0.0f, 0.0f,
                    0.0f,       2.0f/(T-B),       0.0f, 0.0f,
                    0.0f,             0.0f,       0.5f, 0.0f,
                    (L+R)/(L-R), (T+B)/(B-T),    0.5f, 1.0f,
                };
                g_pd3dDevice->lpVtbl->SetTexture(g_pd3dDevice, 0, (LPDIRECT3DBASETEXTURE8)pcmd->TextureId);
                g_pd3dDevice->lpVtbl->SetViewport(g_pd3dDevice, &viewport);
                g_pd3dDevice->lpVtbl->SetTransform(g_pd3dDevice, D3DTS_PROJECTION, &matProj);
                g_pd3dDevice->lpVtbl->SetIndices(g_pd3dDevice, (IDirect3DIndexBuffer8*)g_pIB, vtx_offset);
                g_pd3dDevice->lpVtbl->DrawIndexedPrimitive(g_pd3dDevice, D3DPT_TRIANGLELIST, 0,
                    (UINT)cmd_list->VtxBuffer.Size, idx_offset, pcmd->ElemCount / 3);
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }

    ImplDX8_StateRestore();
}

bool ImGui_ImplDX8_CreateDeviceObjects()
{
    if (!g_pd3dDevice) return false;
    return ImplDX8_CreateFontsTexture();
}

void ImGui_ImplDX8_InvalidateDeviceObjects()
{
    if (!g_pd3dDevice) return;
    if (g_pVB) { g_pVB->lpVtbl->Release(g_pVB); g_pVB = NULL; }
    if (g_pIB) { g_pIB->lpVtbl->Release(g_pIB); g_pIB = NULL; }
    if (g_FontTexture) {
        g_FontTexture->lpVtbl->Release(g_FontTexture);
        g_FontTexture = NULL;
        ImGui::GetIO().Fonts->TexID = NULL;
    }
}
