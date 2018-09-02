#ifndef _SFX_H_GS_2011_
#define _SFX_H_GS_2011_
#include "Sys.hpp"

enum {
  SFXMaxChannels = 32,
  SFXFmtS16 = 0,
  SFXNoAttack = -1,
  SFXNoRelease = -1,
  SFXLoopsForever = -1,
  SFXEnd
};
SysS32 SFXSam(SysS32 Channel, SysU32 SamFmt, void *SamMem, SysS32 Samples);
SysS32 SFXSetCurrentOffset(SysS32 Channel, SysS32 Offset);
SysS32 SFXGetCurrentOffset(SysS32 Channel);
SysS32 SFXLoops(SysS32 Channel, SysS32 OffsetStart, SysS32 OffsetEnd,
                SysS32 Loops);
SysS32 SFXFreq(SysS32 Channel, SysF32 FreqSamplesPerSecond);
SysS32 SFXVol(SysS32 Channel, SysF32 Left, SysF32 Right);
SysS32 SFXMasterVol(SysF32 Left, SysF32 Right);
SysS32 SFXActivate(SysS32 Channel, SysF32 AttackSeconds);
SysS32 SFXDeActivate(SysS32 Channel, SysF32 ReleaseSeconds);
SysS32 SFXFirstFreeChannel(SysU32 Mask);


SysF32 SFXEnvADSR(SysF32 a, SysF32 d, SysF32 s, SysF32 r, SysF32 t);
SysF32 SFXModSine(SysF32 f, SysF32 t);
SysF32 SFXModPulse(SysF32 f, SysF32 t);
SysF32 SFXModSquare(SysF32 f, SysF32 t);
SysF32 SFXModTri(SysF32 f, SysF32 t);
SysF32 SFXModSaw(SysF32 f, SysF32 t);
SysF32 SFXModNoise(SysF32 f, SysF32 t);
SysF32 SFXModOne(SysF32 f, SysF32 t);
SysF32 SFXFToRC(SysF32 f);
SysF32 SFXHighPassFilter(SysF32 RC, SysF32 s, SysF32 t);
SysF32 SFXLowPassFilter(SysF32 RC, SysF32 s, SysF32 t);

enum {
  SFXSynAdd = (1 << 8),
  SFXSynClip = (1 << 9),
  SFXSynFit = (1 << 10),
  SFXSyn2S16 = (1 << 11),
  SFXSynEnd
};
typedef SysF32 (*SFXSynFn)(SysF32 t);
SysS32 SFXSyn(SysU32 Flags, SFXSynFn Fn, SysF32 *Sam, SysS32 Samples,
              SysS16 *Sam16);

struct SFX3DS {
  SysF32 *Pos, *Vel, *Up, Radius;
};
extern SFX3DS SFX3DListener;
enum { SFX3DWrap = (1 << 0), SFX3DEnd };
SysS32 SFX3D(SysU32 Flags, SysS32 Channel, SFX3DS *Source, SysF32 Volume);

enum {
  FXOff,
  FXStart,
  FXFinish,
  FXFadeOff,
  FXThrust = 0,
  FXLaser,
  FXMAN,
  FXExplosion,
  FXHyper,
  FXBonus,
  FXBump,
  FXTest,
  FXMax
};
SysS32 FXPlay(SysS32 *ID, SysU32 State, SysS32 Effect, SysU32 ChannelMask);
#endif
