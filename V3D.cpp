#include "GFX.hpp"
#include "SFX.hpp"
#include "Sys.hpp"
#include <math.h>


SysU64 XS128PS[2];
SysU64 XS128P(void) {
  SysU64 x = XS128PS[0];
  SysU64 const y = XS128PS[1];
  XS128PS[0] = y;
  x ^= x << 23;
  XS128PS[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
  return (XS128PS[1] + y);
}

SysU64 RNG(void) { return XS128P(); }

SysF64 FHash(SysU64 h, SysF32 Low, SysF32 High) {
  SysF64 l = ((SysF64)(h & 0x7fffffffffffffff)) / 0x7fffffffffffffff;
  return Low + (l * (High - Low));
}

SysF64 FHash(SysF32 Low, SysF32 High) {
  return FHash(RNG(), Low, High);
  ;
}

SysF32 *ScaleMatrix(SysF32 *M,SysF32 Scale)
{
    int o=0;
    for(int j=0;j<4;j++)
    for(int i=0;i<4;i++)
        M[o++]=((i==j)*Scale);
    M[15]=1;
    return M;
}

SysF32 Pi = acosf(-1);

SysF32 ProjM[16] = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -2, 0};

SysF32 IM[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

SysF32 *AxisRotationMatrix(SysF32 *M4x4, const SysF32 *Axis, SysF32 Angle) {
  SysF32 axis[3];
  V3Normalize(axis, Axis);
  SysF32 s = sinf(Angle);
  SysF32 c = cosf(Angle);
  SysF32 oc = 1.0 - c;

  SysF32 m[16] = {oc * axis[0] * axis[0] + c,
                  oc * axis[0] * axis[1] - axis[2] * s,
                  oc * axis[2] * axis[0] + axis[1] * s,
                  0.0,
                  oc * axis[0] * axis[1] + axis[2] * s,
                  oc * axis[1] * axis[1] + c,
                  oc * axis[1] * axis[2] - axis[0] * s,
                  0.0,
                  oc * axis[2] * axis[0] - axis[1] * s,
                  oc * axis[1] * axis[2] + axis[0] * s,
                  oc * axis[2] * axis[2] + c,
                  0.0,
                  0.0,
                  0.0,
                  0.0,
                  1.0};
  M4Copy(M4x4, m);
  return M4x4;
}

SysU32 ScreenW,ScreenH;
void UpdateScreenWH(SysF32 FOVAngle) {
  SysU32 w, h;
  SysF32 a = GFXAspectRatio(&w, &h),d=1.0f/(tanf(FOVAngle/2));
  GFXViewPort(0, 0, w, h);
  GFXClip(0, 0, 0, 0);
  if (a < 1) {
    ProjM[0] = 0;
    ProjM[1] = -d;
    ProjM[4] = d / a;
    ProjM[5] = 0;
  } else {
    ProjM[0] = d;
    ProjM[1] = 0;
    ProjM[4] = 0;
    ProjM[5] = d*a;
  }
  ScreenW=w;ScreenH=h;
}
void CreateRodBuffers(SysS32 Segments,SysF32 CapScale,const GFXV *V,const SysU16 *Index,SysS32 Rods,GFXBVI *vib,SysS32 *Verts,SysS32 *Indices)
{
	const SysS32 SingleRodVerts=(Segments*2+2);
	*Verts=Rods*SingleRodVerts;
	const SysS32 SingleRodIndices=(Segments*(6+2*3));
	*Indices=Rods*SingleRodIndices;
	GFXV *v=(GFXV *)SysScratchMemory;;
	SysU16 *i=(SysU16 *)(&SysScratchMemory[(*Verts)*sizeof(GFXV)]);
	SysS32 vo=0,io=0;
	SysF32 RotM[4*4];
	for(int r=0;r<Rods;r++)
	{
		const GFXV rv[2]={V[Index[r*2]],V[Index[r*2+1]]};
		GFXV dv,nv;
		SysF32 rndv[3]={7477,-2953,1021};
		V3Sub(&(dv.x),&(rv[1].x),&(rv[0].x));
		V3Sub(rndv,rndv,&(rv[0].x));
		V3Cross(rndv,rndv,&(dv.x));
		V3Normalize(&(nv.x),rndv);
		SysS32 vocheck=vo;
		v[vo]=rv[0];
		V3SMul(&(v[vo].u),&(dv.x),-v[vo].t*CapScale);
		v[vo].t=0;
		vo++;
		
		v[vo]=rv[1];
		V3SMul(&(v[vo].u),&(dv.x),v[vo].t*CapScale);
		v[vo].t=0;
		vo++;
		
		for(int s=0;s<Segments;s++)
		{
		  AxisRotationMatrix(RotM,&(dv.x),s*(acosf(-1)*2)/Segments);
		  SysF32 vn[3];
		  M4V3Mul(vn,RotM,&(nv.x));
		  v[vo]=rv[0];
		  V3SMul(&(v[vo].u),vn,v[vo].t);
		  v[vo].t=0;
		  vo++;
		  v[vo]=rv[1];
		  V3SMul(&(v[vo].u),vn,v[vo].t);
		  v[vo].t=0;
		  vo++;
		}
		SysAssert(SingleRodVerts==(vo-vocheck));
		SysS32 iocheck=io;
		for(int s=0;s<Segments;s++)
		{
		  SysS32 s1=(s+1)%Segments;
		  i[io++]=vocheck+0;i[io++]=vocheck+2+s*2;i[io++]=vocheck+2*s1+2;
		  i[io++]=vocheck+2+s*2+1;i[io++]=vocheck+2+s*2+0;i[io++]=vocheck+2*s1+2+0;
		  i[io++]=vocheck+2+s*2+1;i[io++]=vocheck+2+s1*2+0;i[io++]=vocheck+2*s1+2+1;
		  i[io++]=vocheck+1;i[io++]=vocheck+2+s*2+1;i[io++]=vocheck+2*s1+2+1;
		}
		SysAssert(SingleRodIndices==(io-iocheck));
	}
	for(int j=0;j<*Verts;j++)
	{
		if(!(j%SingleRodVerts)) SysODS("\n");
		SysODS("Vert %4d: %f %f,%f, 0x%08x, %f,%f,%f,%f   ",j,
		v[j].x,v[j].y,v[j].z, v[j].rgba, v[j].u, v[j].v, v[j].s, v[j].t);

	}
	SysODS("Indices:\n");
	for(int j=0;j<*Indices;j++)
	{
		if(!(j%SingleRodIndices)) SysODS("\n");
		SysODS("%d ",i[j]);
	}
	SysODS("\n");
	GFXBufferVerts(vib, v, Rods*SingleRodVerts);
	GFXBufferIndices(vib, i, Rods*SingleRodIndices);
}
void RenderCubes(int n, int s, int t, bool LineMode, int LineWidth,
                 SysF32 Opacity, SysU32 RGB = 0xffffff, SysU32 A = 0xff) {
  GFXShaderM4(0, ProjM);
  SysU32 RGBA = (A << 24) | RGB;
  SysF32 lw;
  if(ScreenW>ScreenH) lw=2.0f/ScreenW; else lw=2.0f/ScreenH;
  static GFXV vb[8] = {
      {-1, -1, 1, RGBA, 0, 0, 0, 0},  {1, -1, 1, RGBA, 0, 0, 0, 0},
      {1, 1, 1, RGBA, 0, 0, 0, 0},    {-1, 1, 1, RGBA, 0, 0, 0, 0},

      {-1, -1, -1, RGBA, 0, 0, 0, 0}, {1, -1, -1, RGBA, 0, 0, 0, 0},
      {1, 1, -1, RGBA, 0, 0, 0, 0},   {-1, 1, -1, RGBA, 0, 0, 0, 0},
  };
  const int ibln = 6 * 2 * 3;
  static const SysU16 ibl[ibln] = {
      0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
      1, 2, 6, 1, 6, 5, 2, 3, 7, 2, 7, 6, 0, 3, 7, 0, 7, 4,
  };
  const int iblln = 4 * 2 * 3;
  static const SysU16 ibll[iblln] = {
      0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7,
  };

  static GFXBVI bvi[2] = {{0, 0, 0}, {0, 0, 0}};
  static SysS32 RodsVerts,RodsIndices;

  static int Init = 0;
  if (!Init) {
  	for(int i=0;i<8;i++) vb[i].t=1;
	CreateRodBuffers(8,0,vb,ibll,iblln>>1,&bvi[1],&RodsVerts,&RodsIndices);
	GFXBufferVerts(&bvi[0], vb, 8);
	GFXBufferIndices(&bvi[0], ibl, ibln);
	Init = 1;
  }

  SysF32 m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  XS128PS[0] = s | 1;
  XS128PS[1] = (~s) | (1 << 16);

  SysF32 v[3];
      
  lw*=LineWidth;

  for (int i = 0; i < n; i++) {
    v[0] = FHash(-1, 1);
    v[1] = FHash(-1, 1);
    v[2] = FHash(-1, 1);
    AxisRotationMatrix(m, v, (((t&0xfff)/SysF32(0xfff))  * 2 * acos(-1)));
    m[12] = FHash(-8, 8);
    m[13] = FHash(-8, 8);
    m[14] = FHash(-3, -100);
    SysF32 cm[16];
    M4Mul(cm, m, ProjM);
    GFXShaderM4(0, cm);
    SysF64 rgba[4] = {FHash(0, 1), FHash(0, 1), FHash(0, 1), FHash(0, 1)};
    if (LineMode) {
      GFXShaderV4(0, Opacity * rgba[0], Opacity * rgba[1], Opacity * rgba[2],
                  Opacity * rgba[3]);
      GFXShaderV4(1,lw,lw,lw,1);
      //GFXVerts(GFX_LINES, RodsVerts, RodsIndices, &bvi[1]);
      GFXVerts(GFX_TRIANGLES, RodsVerts, RodsIndices, &bvi[1]);
    } else {
      GFXShaderV4(0, Opacity, Opacity, Opacity, Opacity);
      GFXShaderV4(1,0,0,0,0);
      GFXVerts(GFX_TRIANGLES, 8, ibln, &bvi[0]);
    }
  }
}

#define FONTRODS 1
void RenderText(int n,int s,int t,const SysF32 *ScaleM,SysF32 LineWidth)
{
    static GFXBVI bvi = {0, 0, 0};
    static SysS32 Vs,Is,W,H;

    static int Init = 0;
    if (!Init) {
        GFXV *TxtV;
        SysU16 *TxtI;
        GFXText("1,2,3,4... Cubes abound!",&TxtV,&Vs,&TxtI,&Is,&W,&H);
#if FONTRODS
	for(int i=0;i<Vs;i++) TxtV[i].t=20;
	CreateRodBuffers(8,0,TxtV,TxtI,Is>>1,&bvi,&Vs,&Is);
#else
        GFXBufferVerts(&bvi, TxtV, Vs);
        GFXBufferIndices(&bvi,TxtI,Is);
#endif
        Init = 1;
    }
    SysF32 m[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    XS128PS[0] = s | 1;
    XS128PS[1] = (~s) | (1 << 16);

    SysF32 v[3];

    for (int i = 0; i < n; i++) {
        v[0] = FHash(-1, 1);
        v[1] = FHash(-1, 1);
        v[2] = FHash(-1, 1);
        AxisRotationMatrix(m, v, (((t&0xfff)/SysF32(0xfff))  * 2 * acos(-1)));
        m[12] = FHash(-8, 8);
        m[13] = FHash(-8, 8);
        m[14] = FHash(-3, -100);
        SysF32 cm[16];
        M4Mul(cm, m, ProjM);
        GFXShaderM4(0, cm);
        M4Mul(cm, m, ProjM);
        M4Mul(cm, ScaleM, cm);
        GFXShaderM4(0, cm);
        GFXShaderV4(0, 1, 1, 1, 1);
        SysF64 rgba[4] = {FHash(0, 1), FHash(0, 1), FHash(0, 1), FHash(0, 1)};
        GFXShaderV4(0, rgba[0], rgba[1], rgba[2], rgba[3]);
#if FONTRODS
        SysF32 lw;
        if(ScreenW>ScreenH) lw=2.0f/ScreenW; else lw=2.0f/ScreenH;
	lw*=LineWidth;
        GFXShaderV4(1,lw,lw,lw,1);
        GFXVerts(GFX_TRIANGLES,Vs,Is,&bvi);
#else
        GFXVerts(GFX_LINES,Vs,Is,&bvi);
#endif
    }
}
 
SysF32 GridMod(SysF32 a, SysF32 b) {
  a = fmodf(a, 2 * b);
  if (a > b) {
    a = -b + a - ((SysS32)(a / b)) * b;
  } else if (a < -b) {
    a = b + a - ((SysS32)(a / b)) * b;
  }
  return a;
}

void Limit(SysF32 *v, SysF32 *o, SysF32 l) {
  if (o)
    V3Sub(v, v, o);
  v[0] = GridMod(v[0], l);
  v[2] = GridMod(v[2], l);
  if (o)
    V3Add(v, v, o);
}

SysS32 SysUserInput(SysU32 CmdFlgs, SysU32 ID, SysS32 Xr, SysS32 Yr, SysS32 Zr,
                    SysS32 Pressure) {
  return 0;
}
SysS32 PowOf2(SysS32 v)
{
    SysS32 n=0;
    while((1<<n)<v) n++;
    return n;
}


SysS32 SysUserFrame(SysS32 Flags) {
  static SysU32 Mode = 0;
  if (Mode) {
    if (Flags & SYSFRAME_GL_CREATE) {
      GFXOpen();
    }
  }
  if (Flags & SYSFRAME_RESUME) {
  } else if (Flags & SYSFRAME_PAUSE) {
    return 0;
  }

  switch (Mode) {
  case 0: {
    GFXOpen();
    GFXCull(GFXCULL_NONE);
    GFXRenderBegin();
    GFXClear(0);
    SysState(SYSSTATE_AUDIOON, 0);
    SysState(SYSSTATE_UDPON, 0);
    Mode++;
    break;
  }
  case 1: {
    UpdateScreenWH(2*acosf(-1)*90/360);
    static SysU32 State = 0, LastMs = 0;
    static int fc = 0;
    static SysF32 BlendFactor = 0.5f;
    static SysF32 lw = 16;
    static int NC = (1 << 7);
    const int NCA = 8;
    GFXRenderBegin();
    for (int i = 0; i < 5; i++) {
      if (SysKeyState[30 + i]) {
        State &= ~(1 << i);
        State |= (SysKeyState[225] << i);
      }
    }
    if (SysKeyState[35 + 0]) {
      if (SysKeyState[225])
        BlendFactor += 0.01f;
      else
        BlendFactor -= 0.01f;
      if (BlendFactor < 0)
        BlendFactor = 0;
      else if (BlendFactor > 1)
        BlendFactor = 1;
      SysODS("Objects:%d Blend Factor:%f Line Width:%f\n", NC, BlendFactor, lw);
    }
    if (SysKeyState[35 + 1]) {
      if (SysKeyState[225])
        NC += NCA;
      else
        NC -= NCA;
      if (NC < 0)
        NC = 0;
      SysODS("Objects:%d Blend Factor:%f Line Width:%f\n", NC, BlendFactor, lw);
    }
    if (SysKeyState[35 + 2]) {
      if (SysKeyState[225])
        lw ++;
      else
        lw --;
      if (lw < 0)
        lw = 0;
      SysODS("Objects:%d Blend Factor:%f Line Width:%f\n", NC, BlendFactor, lw);
    }
    GFXShader(GFX_STANDARDSHADER);
    GFXBlend(GFXBLEND_OFF, GFXBLEND_OFF, 0);
    GFXRGBAMask(true, true, true, true);
    GFXCull(GFXCULL_NONE);
    GFXTex(0, GFXTEX_NULL);
    GFXZOptions(GFXFLAG_Z_WRITE_ON | GFXFLAG_Z_TEST_OFF);

    if (!(State & 1))
        LastMs = SysU32(SysNSec()/1000000);

    GFXClear(0x0);
    GFXZOptions(GFXFLAG_Z_WRITE_ON | GFXFLAG_Z_TEST_LE);

    if (State & 4)
      GFXBlend(GFXBLEND_ONE, GFXBLEND_ONE, 0);
    else
      GFXBlend(GFXBLEND_OFF, GFXBLEND_OFF, 0);
    GFXRGBAMask(true, true, true, true);
    RenderCubes(NC, 0, LastMs, true, lw, 1);
    GFXRGBAMask(true, true, true, true);
    //GFXZOptions(GFXFLAG_Z_WRITE_OFF | GFXFLAG_Z_TEST_OFF);
    //GFXBlend(GFXBLEND_SRC_COLOR, GFXBLEND_ONE_MINUS_SRC_COLOR, 0);
    GFXBlend(GFXBLEND_OFF, GFXBLEND_OFF, 0);
    SysF32 m[16];
    ScaleMatrix(m,0.1f);
    RenderText((NC*55)/100,100,LastMs,m,lw);
    if (!(State & 8)) {
      GFXZOptions((GFXFLAG_Z_WRITE_OFF) |
                  ((State & 2) ? GFXFLAG_Z_TEST_OFF : GFXFLAG_Z_TEST_L));
      GFXBlend(GFXBLEND_ZERO, GFXBLEND_SRC_COLOR, 0);
      GFXRGBAMask(true, true, true, true);
      RenderCubes(NC,0, LastMs, false, 0, BlendFactor);
    }

    fc++;
    break;
  }
  }

  GFXRenderEnd();

  if (0) {
    GFXClose();
  }

  return 0;
}
