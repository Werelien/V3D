#include "SFX.hpp"
#include "GFX.hpp"
#include <math.h>

static SysU32 Active = 0;
static SysU32 FadeOff = 0;
static SysU32 FadeOn = 0;
static SysF32 MasterVol[2] = {1, 1};

static SysF32 SFXHWFreq = 44100;

SysU8 *MemCpy(SysU8 *d, const SysU8 *s, SysU32 l) {
  for (SysU32 i = 0; i < l; i++)
    d[i] = s[i];
  return d;
}

SysS32 MemRead(SysU8 *b, SysU32 l, const SysU8 *s, SysU32 sl, SysU32 *so) {
  if ((so[0] + l) > sl)
    l = sl - so[0];
  MemCpy(b, &s[so[0]], l);
  so[0] += l;
  return l;
}

SysS32 MemWrite(const SysU8 *b, SysU32 l, SysU8 *m, SysU32 ml, SysU32 *mo) {
  SysAssert((mo[0] + l) <= ml);
  MemCpy(&m[mo[0]], b, l);
  mo[0] += l;
  return l;
}

static struct SFXS {
  SysU32 Fmt;
  void *Sam;
  SysF32 Freq, Vol[2], FadeS[2];
  SysS32 Len, Loops, LoopStart, LoopEnd;
  SysF32 CurrentFadeV, CurrentOffset, CurrentPeriod;
} Ch[SFXMaxChannels];

SysS32 SFXFirstFreeChannel(SysU32 Mask) {
  if ((Active & Mask) == (0xffffffff & Mask))
    return -1;

  SysU32 i = 0;
  while (!((Mask >> i) & 1))
    i++;
  while (((Active & Mask) >> i) & 1)
    i++;
  return i;
}

SysS32 SFXSetCurrentOffset(SysS32 Channel, SysS32 Offset) {
  Ch[Channel].CurrentOffset = Offset;
  return 0;
}

SysS32 SFXGetCurrentOffset(SysS32 Channel) {
  return Ch[Channel].CurrentOffset;
  return 0;
}

SysS32 SFXSam(SysS32 Channel, SysU32 SamFmt, void *SamMem, SysS32 Samples) {
  SysAssert(SamFmt == SFXFmtS16);
  Ch[Channel].Fmt = SamFmt;
  Ch[Channel].Sam = SamMem;
  Ch[Channel].Len = Samples;
  return 0;
}

SysS32 SFXLoops(SysS32 Channel, SysS32 OffsetStart, SysS32 OffsetEnd,
                SysS32 LoopsNgvForever) {
  Ch[Channel].LoopStart = OffsetStart;
  Ch[Channel].LoopEnd = OffsetEnd;
  Ch[Channel].Loops = LoopsNgvForever;
  return 0;
}

SysS32 SFXFreq(SysS32 Channel, SysF32 FreqSamplesPerSecond) {
  Ch[Channel].Freq = FreqSamplesPerSecond;
  Ch[Channel].CurrentPeriod = Ch[Channel].Freq / SFXHWFreq;
  return 0;
}

SysS32 SFXVol(SysS32 Channel, SysF32 Left, SysF32 Right) {
  Ch[Channel].Vol[0] = Left;
  Ch[Channel].Vol[1] = Right;
  return 0;
}

SysS32 SFXMasterVol(SysF32 Left, SysF32 Right) {
  MasterVol[0] = Left;
  MasterVol[1] = Right;
  return 0;
}

SysS32 SFXActivate(SysS32 Channel, SysF32 AttackSeconds) {
  Active &= ~(1 << Channel);
  Ch[Channel].CurrentOffset = 0;
  FadeOff &= ~(1 << Channel);
  if (AttackSeconds > 0) {
    Ch[Channel].FadeS[0] = AttackSeconds;
    FadeOn |= (1 << Channel);
    Ch[Channel].CurrentFadeV = 0.0f;
  } else {
    FadeOn &= ~(1 << Channel);
    Ch[Channel].CurrentFadeV = 1.0f;
  }
  Active |= (1 << Channel);
  return 0;
}

SysS32 SFXDeActivate(SysS32 Channel, SysF32 ReleaseSeconds) {
  if (ReleaseSeconds > 0) {
    Ch[Channel].FadeS[1] = ReleaseSeconds;
    FadeOff |= (1 << Channel);
  } else {
    FadeOff &= ~(1 << Channel);
    Active &= ~(1 << Channel);
  }
  FadeOn &= ~(1 << Channel);
  return 0;
}

SysS32 SFXFadedMix(SysF32 *MixBuffer, SysS32 MixBufferLen, SysS32 c,
                   SysS16 *Sam) {
  SysF32 drv;
  if (FadeOff & (1 << c))
    drv = -1.0f / (Ch[c].FadeS[1] * SFXHWFreq);
  else
    drv = 1.0f / (Ch[c].FadeS[0] * SFXHWFreq);

  SysS32 j = 0;
  do {
    SysS32 LenToEnd =
        (Ch[c].LoopEnd - Ch[c].CurrentOffset) / Ch[c].CurrentPeriod;
    if (LenToEnd > MixBufferLen)
      LenToEnd = MixBufferLen;
    for (SysS32 k = 0; k < LenToEnd; k++) {
      SysU32 oi = Ch[c].CurrentOffset;
      SysF32 s = Sam[oi] * Ch[c].CurrentFadeV;
      Ch[c].CurrentFadeV += drv;
      if (Ch[c].CurrentFadeV < 0)
        return 1;
      else if (Ch[c].CurrentFadeV > 1) {
        Ch[c].CurrentFadeV = 1;
        FadeOn &= ~(1 << c);
        drv = 0;
      }
      SysF32 l = s * Ch[c].Vol[0];
      SysF32 r = s * Ch[c].Vol[1];
      MixBuffer[(j << 1) + 0] += l;
      MixBuffer[(j << 1) + 1] += r;
      j++;
      Ch[c].CurrentOffset += Ch[c].CurrentPeriod;
    }
    if (Ch[c].CurrentOffset >= Ch[c].LoopEnd) {
      if (Ch[c].Loops) {
        Ch[c].CurrentOffset =
            Ch[c].LoopStart + fmodf(Ch[c].CurrentOffset - Ch[c].LoopStart,
                                    (Ch[c].LoopEnd - Ch[c].LoopStart));
        if (Ch[c].Loops > 0)
          Ch[c].Loops--;
      } else if (Ch[c].LoopEnd == Ch[c].Len) {
        return 1;
      } else
        Ch[c].LoopEnd = Ch[c].Len;
    }
    MixBufferLen -= LenToEnd;
  } while (MixBufferLen > 0);
  SysAssert(!MixBufferLen);
  return 0;
}

SysS32 SFXMix(SysF32 *MixBuffer, SysS32 MixBufferLen, SysS32 c, SysS16 *Sam) {
  if ((FadeOff | FadeOn) & (1 << c))
    return SFXFadedMix(MixBuffer, MixBufferLen, c, Sam);

  SysS32 j = 0;
  do {
    SysS32 LenToEnd =
        (Ch[c].LoopEnd - Ch[c].CurrentOffset) / Ch[c].CurrentPeriod;
    if (LenToEnd > MixBufferLen)
      LenToEnd = MixBufferLen;
    for (SysS32 k = 0; k < LenToEnd; k++) {
      SysU32 oi = Ch[c].CurrentOffset;
      SysF32 s = Sam[oi];
      SysF32 l = s * Ch[c].Vol[0];
      SysF32 r = s * Ch[c].Vol[1];
      MixBuffer[(j << 1) + 0] += l;
      MixBuffer[(j << 1) + 1] += r;
      j++;
      Ch[c].CurrentOffset += Ch[c].CurrentPeriod;
    }
    if (Ch[c].CurrentOffset >= Ch[c].LoopEnd) {
      if (Ch[c].Loops) {
        Ch[c].CurrentOffset =
            Ch[c].LoopStart + fmodf(Ch[c].CurrentOffset - Ch[c].LoopStart,
                                    (Ch[c].LoopEnd - Ch[c].LoopStart));
        if (Ch[c].Loops > 0)
          Ch[c].Loops--;
      } else if (Ch[c].LoopEnd == Ch[c].Len) {
        return 1;
      } else
        Ch[c].LoopEnd = Ch[c].Len;
    }
    MixBufferLen -= LenToEnd;
  } while (MixBufferLen > 0);
  SysAssert(!MixBufferLen);
  return 0;
}
static const SysS32 MaxMixBufferSize=(8*1024*1024);
static SysF32 MixBuffer[MaxMixBufferSize];
static SysU32 MixBufferLen = 0;
SysS32 SysUserAudio(SysU32 AudioFlags, SysU32 AudioFreq, SysU8 *Stream,
                    SysS32 Len) {
  // static SysU32 l=0;
  // SysU32 m=SysMs(),d=m-l;
  // if(!d) d=1;
  // SysODS("%d %d %x %d %d %d
  // %d\n",AudioFlags,AudioFreq,Stream,(Len*14)/4,d,1000/d,(Len*1000)/(d*4));
  // l=m;

  SFXHWFreq = AudioFreq;

  if (!Active) {
    for (SysS32 i = 0; i < (Len >> 2); i++)
      ((SysU32 *)Stream)[i] = 0;
    return 0;
  }

  SysU32 BufferLen = (Len >> 2);
  if (MixBufferLen < BufferLen) {
    MixBufferLen = BufferLen;
    SysAssert(MixBufferLen<MaxMixBufferSize);
  }

  for (SysU32 i = 0; i < (MixBufferLen << 1); i++)
    MixBuffer[i] = 0;

  SysU32 i = 0, a = Active;
  while (a) {
    if (a & 1) {
      SysS32 Finished;
      Finished = SFXMix(MixBuffer, MixBufferLen, i, (SysS16 *)(Ch[i].Sam));
      if (Finished)
        SFXDeActivate(i, SFXNoRelease);
    }
    a >>= 1;
    i++;
  }

  SysS16 *b = (SysS16 *)Stream;

  for (SysU32 i = 0; i < MixBufferLen; i++) {
    SysF32 l = MixBuffer[(i << 1) + 0] * MasterVol[0];
    SysF32 r = MixBuffer[(i << 1) + 1] * MasterVol[1];
    if (l < -0x8000)
      l = -0x8000;
    else if (l > 0x7fff)
      l = 0x7fff;
    if (r < -0x8000)
      r = -0x8000;
    else if (r > 0x7fff)
      r = 0x7fff;
    b[(i << 1) + 0] = l;
    b[(i << 1) + 1] = r;
  }

  // SysODS("%08x %08x %08x\n",Active,FadeOn,FadeOff);
  return 0;
}

SysF32 SFXEnvADSR(SysF32 a, SysF32 d, SysF32 s, SysF32 r, SysF32 t) {
  if (t < a)
    return t / a;
  t -= a;
  if (t < d)
    return 1 + (t / d) * (s - 1);
  t -= d;
  SysF32 h = (1 - (a + d + r));
  if (t < h)
    return s;
  t -= h;
  if (t < r)
    return (1 - t / r) * s;
  return 0;
}

SysF32 SFXModSine(SysF32 f, SysF32 t) {
  const SysF32 pi2 = acosf(-1) * 2;
  return sinf(t * f * pi2);
}

SysF32 SFXModSquare(SysF32 f, SysF32 t) {
  t = fmodf(f * t, 1);
  if (t < 0.5f)
    return -1;
  else
    return 1;
}

SysF32 SFXModPulse(SysF32 f, SysF32 t) {
  t = fmodf(f * t, 1);
  if (t < 0.5f)
    return 0;
  else
    return 1;
}

SysF32 SFXModTri(SysF32 f, SysF32 t) {
  t = fmodf(f * t, 1);
  if (t < 0.5f)
    return (-1 + 2 * (t / 0.5f));
  else
    return (1 - 2 * ((t - 0.5f) / 0.5f));
}

SysF32 SFXModSaw(SysF32 f, SysF32 t) {
  t = fmodf(f * t, 1);
  return (-1 + 2 * t);
}

SysF32 SFXModOne(SysF32 f, SysF32 t) { return 1; }

SysU32 Hash(SysU32 Index) { return Index; };
SysF32 SFXNoise(SysU32 s, SysF32 x) {
  SysAssert(x >= 0);
  SysS32 xi[2] = {(SysS32)x, (SysS32)(x + 1)};
  x -= xi[0];
  SysU32 h[2] = {Hash((s << 16) + xi[0]) & 0xffff,
                 Hash((s << 16) + xi[1]) & 0xffff};
  SysF32 l[2] = {-1.0f + h[0] * 2.0f / 0xffff, -1.0f + h[1] * 2.0f / 0xffff};
  return l[0] + (1 - x) * (l[1] - l[0]);
}

SysF32 SFXModNoise(const SysF32 f, SysF32 t) { return SFXNoise(0, t * f); }

SysF32 SFXFToRC(SysF32 f) { return 1.0f / (2.0f * acosf(-1) * f); }

SysF32 SFXHighPassFilter(SysF32 RC, SysF32 s, SysF32 t) {
  static SysF32 LastO = 0, LastS = 0, LastT = 0;
  SysF32 o;
  if (t <= 0)
    o = s;
  else {
    SysF32 dt = t - LastT, ds = s - LastS;
    SysF32 a = RC / (RC + dt);
    o = a * (LastO + ds);
  }
  LastO = o;
  LastS = s;
  LastT = t;
  return o;
}

SysF32 SFXLowPassFilter(SysF32 RC, SysF32 s, SysF32 t) {
  static SysF32 LastO = 0, LastT = 0;
  SysF32 o;
  if (t <= 0)
    o = s;
  else {
    SysF32 dt = t - LastT;
    SysF32 a = dt / (RC + dt);
    o = LastO + a * (s - LastO);
  }
  LastO = o;
  LastT = t;
  return o;
}

SysF32 TestFX(SysF32 t) {
  const SysF32 RC = SFXFToRC(8000);
  SysF32 o = SFXModSine(800, t);
  o = SFXHighPassFilter(RC, o, t);
  return o;
}

SysS32 SFXSyn(SysU32 Flags, SFXSynFn Fn, SysF32 *Sam, SysS32 Samples,
              SysS16 *Sam16) {
  SysAssert(Sam);

  if (!(Flags & SFXSynAdd))
    for (SysS32 i = 0; i < Samples; i++)
      Sam[i] = 0;

  for (SysS32 i = 0; i < Samples; i++) {
    Sam[i] += Fn(((SysF32)i) / Samples);
  }

  if (Flags & SFXSynClip) {
    for (SysS32 i = 0; i < Samples; i++)
      if (Sam[i] > 1)
        Sam[i] = 1;
      else if (Sam[i] < -1)
        Sam[i] = -1;
  }

  if (Flags & SFXSynFit) {
    SysF32 m[2] = {0, 0};
    for (SysS32 i = 0; i < Samples; i++)
      if (Sam[i] > m[1])
        m[1] = Sam[i];
      else if (Sam[i] < m[0])
        m[0] = Sam[i];
    for (SysS32 i = 0; i < Samples; i++)
      Sam[i] = -1 + 2 * ((Sam[i] - m[0]) / (m[1] - m[0]));
  }

  if (Flags & SFXSyn2S16) {
    SysAssert(Sam16);
    for (SysS32 i = 0; i < Samples; i++)
      Sam16[i] = 0x7fff * Sam[i];
  }

  return 0;
}

void Limit(SysF32 *v, SysF32 *o, SysF32 l);
SFX3DS SFX3DListener = {0, 0, 0, 0};
SysS32 SFX3D(SysU32 Flags, SysS32 Channel, SFX3DS *Source, SysF32 Volume) {
  if (Channel < 0)
    return Channel;
  SysAssert(Source);
  SysAssert(Source->Pos);
  SysAssert(SFX3DListener.Pos);
  SysF32 v[3];
  V3Copy(v, Source->Pos);
  if (Flags & SFX3DWrap)
    Limit(v, SFX3DListener.Pos, SFX3DListener.Radius);
  V3Sub(v, v, SFX3DListener.Pos);
  SysF32 d = V3Len(v);
  d = 1 - d / Source->Radius;
  d = d * d * d;
  d = d * Volume;
  if (d < 0)
    d = 0;
  else if (d > 1)
    d = 1;
  SFXVol(Channel, d, d);
  return Channel;
}

SysF32 ThrustFX2(SysF32 t) {
  SysF32 ft = t, bf = 880 * 8;
  return (SFXModNoise(bf, ft) * 0.7f + SFXModNoise(bf * 1.5f, ft) * 0.3f) *
         SFXEnvADSR(0.1f, 0.0f, 1.0f, 0.1f, t) * 0.5f;
}

SysF32 ThrustFX(SysF32 t) {
  SysF32 w = SFXModSine(500, t) * SFXModNoise(1000, t);
  return w;
}

SysF32 LaserFX(SysF32 t) {
  return SFXModSaw(220 * (1 + SFXEnvADSR(0.1f, 0.0f, 1.0f, 0.9f, t)), t) *
         SFXEnvADSR(0.1f, 0.0f, 1.0f, 0.1f, t);
}

SysF32 UFOFX(SysF32 t) {
  return SFXModTri(50 * (1 + 2 * SFXEnvADSR(0.5f, 0.0f, 1.0f, 0.5f, t)), t);
}

SysF32 BumpFX(SysF32 t) {
  SysF32 f = 400;
  return SFXModNoise(3 * f, t) * SFXModTri(f, t) * SFXEnvADSR(0, 0, 1, 1, t);
}

SysF32 ExplosionFX(SysF32 t) {
  SysF32 f = 150;
  return SFXModNoise(3 * f, t) * SFXModTri(f, t) *
         SFXEnvADSR(0.0f, 0.0f, 1.0f, 0.5f, t) * 2.0f;
}

SysF32 HyperFX(SysF32 t) {
  SysF32 af;
  af = 0.6f + 0.4f * SFXModSine(20, t);
  return SFXModSine(110, af * t) * SFXEnvADSR(0.1f, 0.0f, 1.0f, 0.9f, t);
}

SysF32 BonusFX(SysF32 t) {
  SysF32 p = fmod(t * 8, 1);
  if (p < 0.5f)
    p = 1;
  else
    p = 0;
  return SFXModSine(440 * 8, t) * p;
}

SysS32 FXPlay(SysS32 *ID, SysU32 State, SysS32 Effect, SysU32 ChannelMask) {
  // if(Effect!=FXThrust) return -1;// else Effect=FXBump;
  const struct SFXS {
    SFXSynFn Function;
    SysF32 Duration;
    SysS32 Len, Loop[2], Loops;
    SysU32 Flags;
    SysF32 AttackS, ReleaseS;
  } Patch[FXMax] = {
      {ThrustFX,
       2.0f,
       44100,
       {0, 44100},
       SFXLoopsForever,
       (SFXSynClip | SFXSyn2S16),
       0.5f,
       0.5f},
      {LaserFX,
       1 / 4.0f,
       (SysS32)(44100 * 0.5f),
       {0, (SysS32)(44100 * 0.5f)},
       0,
       (SFXSynClip | SFXSyn2S16),
       SFXNoAttack,
       SFXNoRelease},
      {UFOFX,
       1 / 4.0f,
       44100,
       {0, 44100},
       SFXLoopsForever,
       (SFXSynClip | SFXSyn2S16),
       SFXNoAttack,
       SFXNoRelease},
      {ExplosionFX,
       1.00f,
       44100,
       {0, 44100},
       0,
       (SFXSynClip | SFXSyn2S16),
       SFXNoAttack,
       SFXNoRelease},
      {HyperFX,
       2.00f,
       44100,
       {0, 44100},
       0,
       (SFXSynClip | SFXSyn2S16),
       SFXNoAttack,
       SFXNoRelease},
      {BonusFX,
       2.00f,
       44100,
       {0, 44100},
       0,
       (SFXSynClip | SFXSyn2S16),
       SFXNoAttack,
       SFXNoRelease},
      {BumpFX,
       1.00f,
       44100,
       {0, 44100},
       0,
       (SFXSynClip | SFXSyn2S16),
       SFXNoAttack,
       SFXNoRelease},
      {TestFX,
       1.0f,
       44100,
       {0 * 44100, 1 * 44100},
       SFXLoopsForever,
       (SFXSynClip | SFXSyn2S16),
       SFXNoAttack,
       SFXNoRelease},
  };
  static SysS16 Sam[FXMax][44100];
  static SysU32 Flags = 0;

  if (!Flags) {
    SysF32 Work[44100];
    for (SysU32 i = 0; i < FXMax; i++) {
      SFXSyn(Patch[i].Flags, Patch[i].Function, Work, Patch[i].Len, Sam[i]);
    }
    Flags = 1;
  }

  SysAssert((Effect >= 0) && (Effect < FXMax));
  if (State == FXStart) {
    if (ID[0] >= 0) {
      if (Active & (1 << ID[0]))
        return ID[0];
    }
    ID[0] = SFXFirstFreeChannel(ChannelMask);
    if (ID[0] < 0)
      return ID[0];
    SFXSam(ID[0], SFXFmtS16, Sam[Effect], Patch[Effect].Len);
    SFXLoops(ID[0], Patch[Effect].Loop[0], Patch[Effect].Loop[1],
             Patch[Effect].Loops);
    SFXFreq(ID[0], Patch[Effect].Len / Patch[Effect].Duration);
    SFXVol(ID[0], 1, 1);
    SFXActivate(ID[0], Patch[Effect].AttackS);
    return ID[0];
  }

  if (ID[0] < 0)
    return ID[0];

  SysAssert(ID[0] < 32);

  if (State == FXFadeOff) {
    SFXDeActivate(ID[0], Patch[Effect].ReleaseS);
    ID[0] = -1;
    return ID[0];
  }

  if (State == FXFinish) {
    Ch[ID[0]].Loops = 0;
    ID[0] = -1;
    return ID[0];
  }

  if (State == FXOff) {
    SFXDeActivate(ID[0], SFXNoRelease);
    ID[0] = -1;
    return ID[0];
  }

  return -1;
}
