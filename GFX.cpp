#include "GFX.hpp"
#include "GFXFont.hpp"
#include "Strings.hpp"

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
#else
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
#endif

#include <math.h>

SysS32 GFXTxtLen(const SysC8 *t) {
  SysS32 i = 0;
  while (t[i])
    i++;
  return i;
}

void GFXTxtInt(SysC8 *t, SysS32 n, SysS32 k) {
  SysAssert(n >= 0);
  while (n) {
    *t-- = (n % 10) + '0';
    n /= 10;
    k--;
    if (k < 0)
      return;
  }
  while (k) {
    *t-- = '0';
    k--;
  }
}

SysF32 GFXLineWidthRange(SysF32 *r) {
  SysAssert(0);
  glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, r);
  return r[0];
}

SysS32 GFXScreenShot(void *m, SysU32 w, SysU32 h) {
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, m);
  return w * h * 2;
}

static const SysU32 MaxMats = 16;
static const SysU32 VA[3] = {0, 1, 2};
static const SysU32 MaxVTMS = 16, MaxVUAS = 4 + MaxVTMS, MaxFTUS = 4,
                    MaxFUAS = 4;
static SysU32 ShadersActive = 0;
static struct VFUS {
  SysS32 VTM[MaxVTMS], VUA[MaxVUAS], FTU[MaxFTUS], FUA[MaxFUAS];
} SU[GFX_MAXSHADERS];
static SysU32 VShader[GFX_MAXSHADERS], FShader[GFX_MAXSHADERS],
    VFShader[GFX_MAXSHADERS];

const SysU32 MaxInfoLogLen = 16 * 1024;
static void ShaderLog(SysU32 Shader) {
  GLint Status;
  glGetShaderiv(Shader, GL_COMPILE_STATUS, &Status);

  SysC8 *InfoLog = (SysC8 *)SysScratchMemory;
  glGetShaderInfoLog(Shader, MaxInfoLogLen, NULL, InfoLog);
  SysODS("ShaderLog(%x):%s\n", Shader, InfoLog);

  SysAssert(Status == GL_TRUE);
}

static void ProgramLog(SysU32 Program) {
  GLint Status;
  glGetProgramiv(Program, GL_LINK_STATUS, &Status);

  SysC8 *InfoLog = (SysC8 *)SysScratchMemory;
  glGetProgramInfoLog(Program, MaxInfoLogLen, NULL, InfoLog);
  SysODS("ProgramLog(%x):%s\n", Program, InfoLog);

  SysAssert(Status == GL_TRUE);
}

static void CheckError(void) {
  static SysU32 Count = 0;
  GLenum e;
  do {
    e = glGetError();
    if (e != GL_NO_ERROR)
      SysODS("E(%d):%x\n", Count++, e);
  } while (e != GL_NO_ERROR);
}

SysF32 GFXAspectRatio(SysU32 *w, SysU32 *h) {
  SysU32 sw, sh;
  SysAspectRatio(&sw, &sh);
  if (w)
    *w = sw;
  if (h)
    *h = sh;
  return ((SysF32)(sw)) / ((SysF32)(sh));
}

static SysU32 GFXDefaultTex, CurrentShader;

void GFXTex(SysU32 Stage, SysU32 TexHandle) {
  glActiveTexture(GL_TEXTURE0 + Stage);
  if (TexHandle == GFXTEX_NULL)
    glBindTexture(GL_TEXTURE_2D, (GLuint)GFXDefaultTex);
  else
    glBindTexture(GL_TEXTURE_2D, (GLuint)TexHandle);
  glUniform1i(SU[CurrentShader].FTU[Stage], Stage);
}

void GFXTexDestroy(SysU32 TexHandle) {
  if (TexHandle != GFXTEX_NULL)
    glDeleteTextures(1, (const GLuint *)&TexHandle);
}

SysU32 GFXTexCreate(SysU32 Flags, SysU32 Width, SysU32 Height, void *Data) {
  SysODS("Tex:%dx%d %x\n", Width, Height, Flags);

  GLuint t;
  glGenTextures(1, &t);
  glBindTexture(GL_TEXTURE_2D, t);
  switch (Flags) {
  case GFXTEX_8888RGBA:
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, Data);
    break;
  case GFXTEX_4444RGBA:
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA,
                 GL_UNSIGNED_SHORT_4_4_4_4, Data);
    break;
  case GFXTEX_5551RGBA:
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA,
                 GL_UNSIGNED_SHORT_5_5_5_1, Data);
    break;
  case GFXTEX_565RGB:
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB,
                   GL_UNSIGNED_SHORT_5_6_5, Data);
      break;
  default:
      SysAssert(0);
  }
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  return (SysU32)t;
}

void GFXRenderBegin(void) {}

void GFXRenderEnd(void) {}

void GFXLineWidth(SysF32 W) { SysAssert(0);glLineWidth(W); }

static const SysS32 TextMaxChars = 1024, MaxTextVerts = TextMaxChars*8*8;
static GFXV TextBV[MaxTextVerts];
static SysU16 TextBI[MaxTextVerts*2];



void GFXText(const SysC8 *Text, GFXV **Vert, SysS32 *Verts, SysU16 **Index, SysS32 *Indices,SysS32 *Width,SysS32 *Height){
  *Vert=TextBV;
  *Index=TextBI;
  SysS32 vs=0,is=0;
  SysS32 i=0,vo=0,io=0,li=0,vc,mh=-255,lh=255;
  while(Text[i])
  {
	  SysODS("Text Char:%c (%d): ",Text[i],SysS32(Text[i]));
	  SysAssert(Text[i]>=32);
	  SysAssert((Text[i]-32)<GFXFontCharacters);
	  SysS32 fo=(Text[i]-32)*GFXFontCharacterBytes;
	  SysS32 cvo=vo;
	  vc=0;
	  for(int j=(fo+2);j<(fo+2+GFXFont[fo+0]*2);j+=2)
	  {
		if((GFXFont[j]==-1)&&(GFXFont[j+1]==-1))
		{
			SysODS("Start new line strip.\n");
			GFXV v={0,0,0,0x5819,0,0,0,0};
			TextBV[vo]=v;
		}
		else
		{
		for(int k=0;k<2;k++) SysODS("%d ",GFXFont[j+k]);
		if(GFXFont[j+1]>mh) mh=GFXFont[j+1];
		if(GFXFont[j+1]<lh) lh=GFXFont[j+1];
		GFXV v={SysF32(li+GFXFont[j+0]),SysF32(GFXFont[j+1]),0,0xffffffff,0,0,0,0};
		TextBV[vo]=v;
	  	vc++;
		}
		vo++;
	  }
	  SysODS("\n");
	  for(int j=cvo+1;j<vo;j++)
	  {
		if(TextBV[j].rgba==0x5819) continue;
		if(TextBV[j-1].rgba==0x5819) continue;
	  	TextBI[io++]=j-1;
	  	TextBI[io++]=j;
	  }
	  SysODS("Char Vertices:%d Scanned %d active.\n",GFXFont[fo+0],vc);
	  li+=GFXFont[fo+1];
	  SysODS("Char Width:%d(%d)\n",GFXFont[fo+1],li);
	  i++;
	  SysAssert(i<TextMaxChars);
  }
  *Verts=vo;
  *Indices=io;
  *Width=li;
  *Height=mh;
}

static SysS32 GFXGetUniformID(SysU32 s, const SysC8 *n, SysU32 i) {
  SysAssert(GFXTxtLen(n) == 1);
  SysC8 t[] = "X[0] ";
  t[0] = n[0];
  if (i < 10) {
    t[2] = '0' + i % 10;
    t[3] = ']';
    t[4] = 0;
  } else {
    t[2] = '0' + i / 10;
    t[3] = '0' + i % 10;
    t[4] = ']';
  }
  SysS32 l = glGetUniformLocation(s, t);
  SysODS("%s:%d ", t, l);
  // SysAssert(l>=0);
  return l;
}

void GFXShaderCreate(SysU32 s, const SysC8 *VSource, const SysC8 *FSource) {
  SysAssert(s < GFX_MAXSHADERS);

  SysAssert(!(ShadersActive & (1 << s)));

  SysODS("\nShader:%d\n", s);

  SysODS("Vertex...\n");
  SysODS("%s\n", VSource);
  VShader[s] = glCreateShader(GL_VERTEX_SHADER);
  SysAssert(VShader[s]);
  glShaderSource(VShader[s], 1, &VSource, 0);
  glCompileShader(VShader[s]);
  ShaderLog(VShader[s]);

  SysODS("Fragment...\n");
  SysODS("%s\n", FSource);
  FShader[s] = glCreateShader(GL_FRAGMENT_SHADER);
  SysAssert(FShader[s]);
  glShaderSource(FShader[s], 1, &FSource, 0);
  glCompileShader(FShader[s]);
  ShaderLog(FShader[s]);

  VFShader[s] = glCreateProgram();
  glAttachShader(VFShader[s], VShader[s]);
  glAttachShader(VFShader[s], FShader[s]);
  glBindAttribLocation(VFShader[s], VA[0], "V");
  glBindAttribLocation(VFShader[s], VA[1], "C");
  glBindAttribLocation(VFShader[s], VA[2], "T");
  glLinkProgram(VFShader[s]);
  ProgramLog(VFShader[s]);

  for (SysU32 i = 0; i < MaxVTMS; i++)
    SU[s].VTM[i] = GFXGetUniformID(VFShader[s], "M", i);
  for (SysU32 i = 0; i < MaxVUAS; i++)
    SU[s].VUA[i] = GFXGetUniformID(VFShader[s], "A", i);
  for (SysU32 i = 0; i < MaxFTUS; i++)
    SU[s].FTU[i] = GFXGetUniformID(VFShader[s], "U", i);
  for (SysU32 i = 0; i < MaxFUAS; i++)
    SU[s].FUA[i] = GFXGetUniformID(VFShader[s], "F", i);
  ShadersActive |= (1 << s);
  CheckError();
  SysODS("\n");
}

void GFXShaderDestroy(SysU32 s) {
  SysAssert(s < GFX_MAXSHADERS);
  SysAssert((ShadersActive & (1 << s)));
  glDeleteProgram(VFShader[s]);
  glDeleteShader(FShader[s]);
  glDeleteShader(VShader[s]);
  ShadersActive &= ~(1 << s);
}

static SysU64 GFXNSecAtOpen = 0;
void GFXOpen(void) {
  if (!GFXNSecAtOpen)
    GFXNSecAtOpen = SysNSec();

  ShadersActive = 0;

  SysODS("OpenGL Extensions:%s\n", (SysC8 *)glGetString(GL_EXTENSIONS));
  SysODS("OpenGL Vendor    :%s\n", (SysC8 *)glGetString(GL_VENDOR));
  SysODS("OpenGL Renderer  :%s\n", (SysC8 *)glGetString(GL_RENDERER));
  SysODS("OpenGL Version   :%s\n", (SysC8 *)glGetString(GL_VERSION));

  SysU32 w, h;
  SysAspectRatio(&w, &h);
  GFXViewPort(0, 0, w, h);
  GFXClip(0, 0, 0, 0);

  while (glGetError() != GL_NO_ERROR)
    ;

  for (SysU32 s = 0; s < GFX_MAXDEFAULTSHADERS; s++) {
    GFXShaderCreate(s, STR3VSrc[s * 2 + 0], STR3VSrc[s * 2 + 1]);
  }

  glEnable(GL_DITHER);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);

  glEnable(GL_DEPTH_TEST);
  glClearDepthf(1);
  GFXZOptions(GFXFLAG_Z_WRITE_ON | GFXFLAG_Z_TEST_G);

  CurrentShader = 0xffffffff;
  GFXShader(GFX_STANDARDSHADER);
  GFXShaderV4(0, 1 / 255.0f, 1 / 255.0f, 1 / 255.0f, 1 / 255.0f);

  SysU32 RGBA = 0xffffffff;
  GFXDefaultTex = GFXTexCreate(GFXTEX_8888RGBA, 1, 1, &RGBA);
  GFXTex(0, GFXTEX_NULL);

  CheckError();
}

void GFXShader(SysU32 Shader) {
  if (CurrentShader == Shader)
    return;
  glUseProgram(VFShader[Shader]);
  glEnableVertexAttribArray(VA[0]);
  glEnableVertexAttribArray(VA[1]);
  glEnableVertexAttribArray(VA[2]);
  CurrentShader = Shader;
}

void GFXShaderV4(SysU32 Index, SysF32 V0, SysF32 V1, SysF32 V2, SysF32 V3) {
  SysAssert((Index & GFXFRAGV4) || (Index < MaxVUAS));
  SysAssert((!(Index & GFXFRAGV4)) || ((Index & (~GFXFRAGV4)) < MaxFUAS));

  if (Index & GFXFRAGV4)
    glUniform4f(SU[CurrentShader].FUA[Index & (MaxFUAS - 1)], V0, V1, V2, V3);
  else
    glUniform4f(SU[CurrentShader].VUA[Index], V0, V1, V2, V3);
}

void GFXShaderM4(SysU32 Index, const SysF32 *M4) {
  SysAssert(Index < MaxVTMS);

  glUniformMatrix4fv(SU[CurrentShader].VTM[Index], 1, GL_FALSE, M4);
}

void GFXViewPort(SysS32 x, SysS32 y, SysS32 w, SysS32 h) {
  glViewport(x, y, w, h);
}

void GFXClose(void) {}

void GFXClip(SysS32 X, SysS32 Y, SysS32 Width, SysS32 Height) {
  if (!(X | Y | Width | Height))
    glDisable(GL_SCISSOR_TEST);
  else {
    glEnable(GL_SCISSOR_TEST);
    glScissor(X, Y, Width, Height);
  }
}

void GFXClear(SysU32 RGBA) {
  glClearColor(((RGBA >> 0) & 0xff) / 255.0f, ((RGBA >> 8) & 0xff) / 255.0f,
               ((RGBA >> 16) & 0xff) / 255.0f, ((RGBA >> 24) & 0xff) / 255.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GFXRGBAMask(bool R, bool G, bool B, bool A) { glColorMask(R, G, B, A); }

void GFXBufferVerts(GFXBVI *BufferVertIndex, const GFXV *Vert, SysS32 Verts,
                    SysU32 Flags, SysS32 UpdateStartIndex,
                    SysS32 UpdateEndIndex) {
  SysAssert((Flags & 1) == 0);
  SysAssert(!BufferVertIndex->VID);
  glGenBuffers(1, &(BufferVertIndex->VID));
  glBindBuffer(GL_ARRAY_BUFFER, BufferVertIndex->VID);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GFXV) * Verts, Vert, GL_STATIC_DRAW);
  BufferVertIndex->Flags |= 1;
}

void GFXBufferIndices(GFXBVI *BufferVertIndex, const SysU16 *Index,
                      SysS32 Indices, SysU32 Flags, SysS32 UpdateStartIndex,
                      SysS32 UpdateEndIndex) {
  SysAssert((Flags & 2) == 0);
  SysAssert(!BufferVertIndex->IID);
  glGenBuffers(1, &(BufferVertIndex->IID));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferVertIndex->IID);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(SysU16) * Indices, Index,
               GL_STATIC_DRAW);
  BufferVertIndex->Flags |= 2;
}

void GFXBufferVertIndexDestroy(GFXBVI *BufferVertIndex) {
  if (BufferVertIndex->Flags & 1)
    glDeleteBuffers(1, &BufferVertIndex->VID);
  if (BufferVertIndex->Flags & 2)
    glDeleteBuffers(1, &BufferVertIndex->IID);
  BufferVertIndex->VID=BufferVertIndex->IID=0;
  BufferVertIndex->Flags = 0;
}

void GFXVerts(SysU32 PrimType, SysS32 Verts, SysS32 Indices,
              const GFXBVI *BufferVertIndex) {
  if (!BufferVertIndex->Flags)
    return;

  glBindBuffer(GL_ARRAY_BUFFER, BufferVertIndex->VID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferVertIndex->IID);
  SysU64 o=0;
  glVertexAttribPointer(VA[0], 3, GL_FLOAT, GL_FALSE, sizeof(GFXV), (void *)o);o+=3*sizeof(SysF32);
  glVertexAttribPointer(VA[1], 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(GFXV),(void *)o);o+=sizeof(SysU32);
  glVertexAttribPointer(VA[2], 4, GL_FLOAT, GL_FALSE, sizeof(GFXV),(void *)o);o=4*sizeof(SysF32);
  if (BufferVertIndex->Flags & 2) {
    glDrawElements(PrimType, Indices, GL_UNSIGNED_SHORT, 0);
  } else {
    glDrawArrays(PrimType, 0, Verts);
  }
}

void GFXZOptions(SysU32 ZFlags) {
  if (ZFlags & GFXFLAG_Z_WRITE_OFF)
    glDepthMask(GL_FALSE);
  else
    glDepthMask(GL_TRUE);

  if (ZFlags & GFXFLAG_Z_TEST_OFF)
    glDisable(GL_DEPTH_TEST);
  else {
    glEnable(GL_DEPTH_TEST);
    if (ZFlags & GFXFLAG_Z_TEST_GE)
      glDepthFunc(GL_GEQUAL);
    else if (ZFlags & GFXFLAG_Z_TEST_G)
      glDepthFunc(GL_GREATER);
    else if (ZFlags & GFXFLAG_Z_TEST_LE)
      glDepthFunc(GL_LEQUAL);
    else
      glDepthFunc(GL_LESS);
  }
}

void GFXBlend(SysU32 BlendSrc, SysU32 BlendDst, SysU32 ConstantRGBA) {

  glBlendColor(((ConstantRGBA >> 0) & 0xff) / 255.0f,
               ((ConstantRGBA >> 8) & 0xff) / 255.0f,
               ((ConstantRGBA >> 16) & 0xff) / 255.0f,
               ((ConstantRGBA >> 24) & 0xff) / 255.0f);
  if ((BlendSrc != GFXBLEND_OFF) && (BlendDst != GFXBLEND_OFF)) {
    glEnable(GL_BLEND);
    glBlendFunc(BlendSrc, BlendDst);
  } else {
    glDisable(GL_BLEND);
  }
}

void GFXCull(SysU32 Mode) {

  switch (Mode) {
  case GFXCULL_NONE:
    glDisable(GL_CULL_FACE);
    break;
  case GFXCULL_CW:
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    break;
  case GFXCULL_CCW:
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    break;
  }
}

static SysU8 *TgaS;
static SysS32 TgaRle, TgaR;
static SysU32 TgaBytes(SysS32 n) {
  SysS32 i;
  static SysU32 b;
  if (TgaR == 0) {
    TgaRle = *TgaS++;
    TgaR = (TgaRle & 0x7f) + 1;
    TgaRle &= 0x80;
    if (TgaRle) {
      b = 0;
      for (i = 0; i < n; i++) {
        b <<= 8;
        b |= *TgaS++;
      }
    }
  }
  if (TgaRle == 0) {
    b = 0;
    for (i = 0; i < n; i++) {
      b <<= 8;
      b |= *TgaS++;
    }
  }
  TgaR--;
  return b;
}

SysF32 V3DP(const SysF32 *a, const SysF32 *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

SysF32 V3Len(const SysF32 *v) { return sqrtf(V3DP(v, v)); }

SysF32 *V3SAdd(SysF32 *d, const SysF32 *s, const SysF32 a) {
  d[0] = s[0] + a;
  d[1] = s[1] + a;
  d[2] = s[2] + a;
  return d;
}

SysF32 *V3SMul(SysF32 *d, const SysF32 *s, const SysF32 m) {
  d[0] = s[0] * m;
  d[1] = s[1] * m;
  d[2] = s[2] * m;
  return d;
}

SysF32 V3Normalize(SysF32 *n, const SysF32 *v) {
  SysF32 r = V3Len(v);
  V3SMul(n, v, 1.0f / r);
  return r;
}

SysF32 *V3Copy(SysF32 *d, const SysF32 *s) {
  d[0] = s[0];
  d[1] = s[1];
  d[2] = s[2];
  return d;
}

SysF32 *V3Sub(SysF32 *d, const SysF32 *b, const SysF32 *a) {
  d[0] = b[0] - a[0];
  d[1] = b[1] - a[1];
  d[2] = b[2] - a[2];
  return d;
}

SysF32 *V3Add(SysF32 *d, const SysF32 *b, const SysF32 *a) {
  d[0] = b[0] + a[0];
  d[1] = b[1] + a[1];
  d[2] = b[2] + a[2];
  return d;
}

SysF32 *M4Copy(SysF32 *d, const SysF32 *s) {
  for (SysU32 i = 0; i < 16; i++)
    d[i] = s[i];
  return d;
}

SysF32 *M4Scale(SysF32 *d, const SysF32 *s, SysF32 scl) {
  for (SysU32 i = 0; i < 16; i++)
    d[i] = s[i] * scl;
  return d;
}

SysF32 *M4Add(SysF32 *d, const SysF32 *a, const SysF32 *b) {
  for (SysU32 i = 0; i < 16; i++)
    d[i] = a[i] + b[i];
  return d;
}

SysF32 *M4Sub(SysF32 *d, const SysF32 *b, const SysF32 *a) {
  for (SysU32 i = 0; i < 16; i++)
    d[i] = b[i] - a[i];
  return d;
}

SysF32 *M4Mul(SysF32 *d, const SysF32 *a, const SysF32 *b) {
  SysF32 r[16];
  r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
  r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
  r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
  r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];
  r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
  r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
  r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
  r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];
  r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
  r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
  r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
  r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];
  r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
  r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
  r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
  r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
  M4Copy(d, r);
  return d;
}

SysF32 *M4V3Mul(SysF32 *d, const SysF32 *m, const SysF32 *v) {
  d[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + m[12];
  d[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + m[13];
  d[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + m[14];
  return d;
}

SysF32 *M3TV3Mul(SysF32 *d, const SysF32 *m, const SysF32 *v) {
  d[0] = v[0] * m[0] + v[1] * m[1] + v[2] * m[2];
  d[1] = v[0] * m[4] + v[1] * m[5] + v[2] * m[6];
  d[2] = v[0] * m[8] + v[1] * m[9] + v[2] * m[10];
  return d;
}

SysF32 *V3Cross(SysF32 *d, const SysF32 *a, const SysF32 *b) {
  d[0] = a[1] * b[2] - a[2] * b[1];
  d[1] = a[2] * b[0] - a[0] * b[2];
  d[2] = a[0] * b[1] - a[1] * b[0];
  return d;
}

SysF32 *M4LookAt(SysF32 *d, const SysF32 *f, const SysF32 *t, const SysF32 *u) {

  SysF32 x[3], y[3], z[3];
  V3Normalize(z, V3Sub(z, f, t));
  V3Normalize(x, V3Cross(x, u, z));
  V3Normalize(y, V3Cross(y, z, x));
  SysF32 m[16] = {x[0], y[0], z[0], 0, x[1], y[1], z[1], 0,
                  x[2], y[2], z[2], 0, 0,    0,    0,    1};
  SysF32 tx[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -f[0], -f[1], -f[2], 1};
  M4Mul(d, tx, m);
  return d;
}
