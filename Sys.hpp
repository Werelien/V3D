#ifndef _SYS_H_GS_2011_
#define _SYS_H_GS_2011_
typedef char SysC8;
typedef unsigned char SysU8;
typedef signed char SysS8;
typedef unsigned short SysU16;
typedef signed short SysS16;
typedef unsigned int SysU32;
typedef unsigned long long SysU64;
typedef signed long long SysS64;
typedef signed int SysS32;
typedef float SysF32;
typedef double SysF64;
enum {
  SYSINPUT_TOUCHON = 0x10000,
  SYSINPUT_TOUCHOFF = 0x10001,
  SYSINPUT_TOUCHMOVE = 0x10002,
  SYSINPUT_ACCEL = 0x20000,
  SYSSTATE_AUDIOON = 1,
  SYSSTATE_AUDIOOFF = -1,
  SYSSTATE_UDPON = 2,
  SYSSTATE_UDPOFF = -2,
  SYSTATE_TITLE = 3,
  SYSAUDIO_STEREO = (1 << 0),
  SYSFRAME_QUIT = (1 << 0),
  SYSFRAME_PAUSE = (1 << 0),
  SYSFRAME_RESUME = (1 << 1),
  SYSFRAME_GL_CREATE = (1 << 2),
  SYSFRAME_GL_CHANGE = (1 << 3),
  SYSLOADSAVE_DEFAULTDIR = 0,
  SYSLOADSAVE_USERDIR = (1 << 1),
  SYSTXTSTYLE_BOLD = 0,
  SYSTXTSTYLE_ITALIC,
  SYSTXTSTYLE_UNDERLINE,
  SYSTXTSTYLE_STRIKETHROUGH,
  SYSTXTSTYLE_NORMAL,
  SYS_LAST
};

#define SYS_MAX_SCRATCH_MEMORY (8*1024*1024)
extern SysU8 SysScratchMemory[];

SysU32 SysState(SysS32 Cmd, void *Arg);
SysU32 SysUDPIP(SysC8 *IPDNSString);
SysU32 SysUDPBroadcastIP(void);
SysU32 SysUDPSend(SysU32 IPAddress, SysU16 Port, void *Packet,
                  SysU32 PacketByteSize);
SysU32 SysUDPBind(SysU32 IPAddress, SysU16 Port);
SysU32 SysUDPRecieve(SysU32 *IPAddress, SysU16 *Port, void *Packet,
                     SysU32 PacketByteSize, SysU32 *BytesRecieved);
SysU64 SysNSec(void);
SysU64 SysRNG(void);
SysS32 SysLoad(SysU32 Flags, const SysC8 *FileName, SysU32 Offset, SysU32 Size,
               void *Buffer);
SysS32 SysSave(SysU32 Flags, const SysC8 *FileName, SysU32 Size, void *Buffer);
SysF32 SysAspectRatio(SysU32 *Width, SysU32 *Height);
SysS32 SysUserAudio(SysU32 Flags, SysU32 AudioFreq, SysU8 *Stream, SysS32 Len);
SysS32 SysUserInput(SysU32 Flags, SysU32 ID, SysS32 X, SysS32 Y, SysS32 Z,
                    SysS32 Pressure);
SysS32 SysUserFrame(SysS32 Flags);
extern const SysU8 *SysKeyState;
void SysKill(SysS32 n);

#ifdef SYS_DEBUG_ODS
void SysODS(const SysC8 *DebugString, ...);
#define SysAssert(exp)                                                         \
  if (!(exp)) {                                                                \
      SysODS("%s:%d\n", __FILE__, __LINE__);    \
    SysKill(0);                                                                \
  }
#else
inline void SysODS(const SysC8 *, ...){};
#define SysAssert(exp)
#endif

#endif
