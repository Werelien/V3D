#ifndef _GFX_H_GS_2011_
#define _GFX_H_GS_2011_
#include "Sys.hpp"

enum {
  GFX_POINTS = 0,
  GFX_LINES = 1,
  GFX_LINE_STRIP = 3,
  GFX_TRIANGLES = 4,
  GFX_TRIANGLE_STRIP = 5,
  GFX_TRIANGLE_FAN = 6
};

enum {
  GFXTEX_NULL = 0,

  GFXTEX_8888RGBA = 1,
  GFXTEX_4444RGBA,
  GFXTEX_5551RGBA,
  GFXTEX_565RGB,

  GFXFLAG_OFF = 0,
  GFXFLAG_ON = 1,

  GFXFLAG_Z_TEST_OFF = (1 << 0),
  GFXFLAG_Z_TEST_GE = (1 << 1),
  GFXFLAG_Z_TEST_G = (1 << 2),
  GFXFLAG_Z_TEST_LE = (1 << 3),
  GFXFLAG_Z_TEST_L = (1 << 4),
  GFXFLAG_Z_WRITE_ON = (1 << 3),
  GFXFLAG_Z_WRITE_OFF = (1 << 4),

  GFXTGA_FLAGFLIPV = (1 << 16),
  GFXTGA_FILELOAD = (1 << 17),
  GFXTGA_ALPHAGEN = (1 << 18),
  GFXTGA_ALPHAGENINV = (1 << 19),
  GFXTGA_INVERT = (1 << 20),
  GFXTGA_TRANS0 = (1 << 21),

  GFXBLEND_ZERO = 0x0,
  GFXBLEND_ONE = 0x1,
  GFXBLEND_SRC_COLOR = 0x0300,
  GFXBLEND_ONE_MINUS_SRC_COLOR = 0x0301,
  GFXBLEND_SRC_ALPHA = 0x0302,
  GFXBLEND_ONE_MINUS_SRC_ALPHA = 0x0303,
  GFXBLEND_DST_ALPHA = 0x0304,
  GFXBLEND_ONE_MINUS_DST_ALPHA = 0x0305,
  GFXBLEND_DST_COLOR = 0x0306,
  GFXBLEND_ONE_MINUS_DST_COLOR = 0x0307,
  GFXBLEND_SRC_ALPHA_SATURATE = 0x0308,
  GFXBLEND_CONSTANT_COLOR = 0x8001,
  GFXBLEND_ONE_MINUS_CONSTANT_COLOR = 0x8002,
  GFXBLEND_CONSTANT_ALPHA = 0x8003,
  GFXBLEND_ONE_MINUS_CONSTANT_ALPHA = 0x8004,
  GFXBLEND_OFF = 0xffff,

  GFXFRAGV4 = (1 << 16),

  GFXCULL_NONE = 0,
  GFXCULL_CW = 1,
  GFXCULL_CCW = 2,

  GFX_STANDARDSHADER = 0,
  GFX_MAXDEFAULTSHADERS,
  GFX_MAXSHADERS = 32,

  GFX_BVI_CREATE_ONCE = 0,

  GFXFLAG_END
};

struct GFXV {
  SysF32 x, y, z;
  SysU32 rgba;
  SysF32 u, v, s, t;
};

struct GFXBVI {
  SysU32 Flags;
  SysU32 VID, IID;
};

void GFXOpen(void);
void GFXClose(void);
SysF32 *GFXAxisRotationMatrix(SysF32 *M4x4, const SysF32 *Axis, SysF32 Angle);
void GFXCreateRods(SysS32 Segments,SysF32 CapScale,const GFXV *V,const SysU16 *Index,SysS32 Rods,GFXBVI *vib,SysS32 *Verts,SysS32 *Indices);
SysF32 GFXLineWidthRange(SysF32 *Range);
void GFXViewPort(SysS32 X, SysS32 Y, SysS32 Width, SysS32 Height);
void GFXClear(SysU32 RGBA);
void GFXShadrCreate(SysU32 Index, const SysC8 *VSource, const SysC8 *FSource);
void GFXShaderDestroy(SysU32 Index);
void GFXShader(SysU32 Index);
void GFXShaderM4(SysU32 Index, const SysF32 *M4);
void GFXShaderV4(SysU32 Index, SysF32 V0, SysF32 V1, SysF32 V2, SysF32 V3);
void GFXClip(SysS32 X, SysS32 Y, SysS32 Width, SysS32 Height);
void GFXBufferVerts(GFXBVI *BufferVertIndex, const GFXV *Vert, SysS32 Verts,
                    SysU32 Flags = GFX_BVI_CREATE_ONCE,
                    SysS32 UpdateStartIndex = 0, SysS32 UpdateEndIndex = 0);
void GFXBufferIndices(GFXBVI *BufferVertIndex, const SysU16 *Index,
                      SysS32 Indices, SysU32 Flags = GFX_BVI_CREATE_ONCE,
                      SysS32 UpdateStartIndex = 0, SysS32 UpdateEndIndex = 0);
void GFXBufferVertIndexDestroy(GFXBVI *BufferVertIndex);
void GFXVerts(SysU32 PrimType, SysS32 Verts, SysS32 Indices,
              const GFXBVI *BufferVertIndex);
void GFXRenderBegin(void);
void GFXRenderEnd(void);
void GFXLineWidth(SysF32 Width);
void GFXCull(SysU32 Mode);
void GFXBlend(SysU32 BlendSrc, SysU32 BlendDst, SysU32 ConstantRGBA);
void GFXZOptions(SysU32 ZFlags);
void GFXTexDestroy(SysU32 TexHandle);
void GFXTex(SysU32 Stage, SysU32 TexHandle);
SysU32 GFXTexCreate(SysU32 Type, SysU32 Width, SysU32 Height, void *Data);
void GFXRGBAMask(bool R, bool G, bool B, bool A);

void GFXText(const SysC8 *Text, GFXV **Vert, SysS32 *Verts, SysU16 **Index, SysS32 *Indices,SysS32 *Width,SysS32 *Height);

SysS32 GFXTxtLen(const SysC8 *t);
void GFXTxtInt(SysC8 *t, SysS32 n, SysS32 k);

SysF32 GFXAspectRatio(SysU32 *w, SysU32 *h);

SysF32 V3DP(const SysF32 *a, const SysF32 *b);
SysF32 V3Len(const SysF32 *v);
SysF32 V3Normalize(SysF32 *n, const SysF32 *v);
SysF32 *V3SAdd(SysF32 *d, const SysF32 *s, const SysF32 a);
SysF32 *V3SMul(SysF32 *d, const SysF32 *s, const SysF32 m);
SysF32 *V3Copy(SysF32 *d, const SysF32 *s);
SysF32 *V3Sub(SysF32 *d, const SysF32 *b, const SysF32 *a);
SysF32 *V3Add(SysF32 *d, const SysF32 *b, const SysF32 *a);
SysF32 *M4Copy(SysF32 *d, const SysF32 *s);
SysF32 *M4Scale(SysF32 *d, const SysF32 *s, SysF32 scl);
SysF32 *M4Add(SysF32 *d, const SysF32 *a, const SysF32 *b);
SysF32 *M4Sub(SysF32 *d, const SysF32 *b, const SysF32 *a);
SysF32 *M4Mul(SysF32 *d, const SysF32 *a, const SysF32 *b);
SysF32 *M4V3Mul(SysF32 *d, const SysF32 *m, const SysF32 *v);
SysF32 *M3TV3Mul(SysF32 *d, const SysF32 *m, const SysF32 *v);
SysF32 *V3Cross(SysF32 *d, const SysF32 *a, const SysF32 *b);
SysF32 *M4LookAt(SysF32 *d, const SysF32 *f, const SysF32 *t, const SysF32 *u);

#endif
