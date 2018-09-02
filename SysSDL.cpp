#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#include <emscripten.h>
#else
#include <SDL2/SDL.h>
#endif

#include "Sys.hpp"
#include <unistd.h>

SysU8 SysScratchMemory[SYS_MAX_SCRATCH_MEMORY];

void SysKill(SysS32 C) { exit(C); }

static const SysU32 SysAudioFreq = 44100, SysAudioFlags = SYSAUDIO_STEREO;
static SysS32 GLRGBAZD[6];
static SysS32 SYS_SCREEN_W = 1920, SYS_SCREEN_H = 1080;

SDL_AudioSpec AudioFmt;
SDL_Window *SysMainWindow;
SDL_GLContext SysMainContext;

static SysU64 XS128PS[2] = {0x2034f234a, 0xc345e342f3e5};
SysU64 SysRNG(void) {
  SysU64 x = XS128PS[0];
  SysU64 const y = XS128PS[1];
  XS128PS[0] = y;
  x ^= x << 23;
  XS128PS[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
  return (XS128PS[1] + y);
}

static void UDPOpen(void) {}

SysU32 SysUDPBroadcastIP(void) { return 0; }

SysU32 SysUDPIP(SysC8 *IPDNSString) { return 0; }

SysU32 SysUDPSend(SysU32 IPAddress, SysU16 Port, void *Packet,
                  SysU32 PacketByteSize) {
  return 0;
}

SysU32 SysUDPBind(SysU32 IPAddress, SysU16 Port) { return 0; }

SysU32 SysUDPRecieve(SysU32 *IPAddress, SysU16 *Port, void *Packet,
                     SysU32 PacketByteSize, SysU32 *BytesRecieved) {
  return 0;
}

static void UDPClose(void) {}

SysF32 SysAspectRatio(SysU32 *Width, SysU32 *Height) {
  if (Width)
    *Width = SYS_SCREEN_W;
  if (Height)
    *Height = SYS_SCREEN_H;
  return SysF32(SYS_SCREEN_W) / SysF32(SYS_SCREEN_H);
}

static SysS32 FlipAxis(SysS32 x) {
  x -= 0x4000;
  x = -x;
  x += 0x4000;
  return x;
}

const SysU8 *SysKeyState = 0;
SysU32 SysPause = 0, SysActive = 0;
SysS32 SysPoll(void) {
  SysRNG();

  SysS32 QuitHit = 0;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {

    switch (event.window.event) {
    case SDL_WINDOWEVENT_FOCUS_GAINED:
      SDL_PauseAudio(0);
      SysActive |= (1 << 1);
      break;
    case SDL_WINDOWEVENT_FOCUS_LOST:
      SDL_PauseAudio(1);
      SysActive |= (1 << 0);
      break;
    }

    SDL_MouseMotionEvent *pm = (SDL_MouseMotionEvent *)&event;
    SDL_MouseButtonEvent *pb = (SDL_MouseButtonEvent *)&event;

    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
      if (pb->button & SDL_BUTTON(SDL_BUTTON_LEFT))
        SysUserInput(SYSINPUT_TOUCHON, 1,
                     FlipAxis((pb->y * 0x7fff) / SYS_SCREEN_H),
                     (pb->x * 0x7fff) / SYS_SCREEN_W, 0, 0x7fff);
      if (pb->button & SDL_BUTTON(SDL_BUTTON_MIDDLE))
        SysUserInput(SYSINPUT_ACCEL, SysNSec(),
                     0x4000 - (pb->x * 0x7fff) / SYS_SCREEN_W,
                     0x4000 - (pb->y * 0x7fff) / SYS_SCREEN_H, 0, 0x7fff);
      break;
    case SDL_MOUSEBUTTONUP:
      if (pb->button == 1)
        SysUserInput(SYSINPUT_TOUCHOFF, 1, (pb->x * 0x7fff) / SYS_SCREEN_W,
                     (pb->y * 0x7fff) / SYS_SCREEN_H, 0, 0x7fff);
      break;
    case SDL_MOUSEMOTION:
      if (pm->state & SDL_BUTTON(SDL_BUTTON_LEFT))
        SysUserInput(SYSINPUT_TOUCHMOVE, 1,
                     FlipAxis((pm->y * 0x7fff) / SYS_SCREEN_H),
                     (pm->x * 0x7fff) / SYS_SCREEN_W, 0, 0x7fff);
      if (pm->state & SDL_BUTTON(SDL_BUTTON_MIDDLE))
        SysUserInput(SYSINPUT_ACCEL, SysNSec(),
                     0x4000 - (pm->x * 0x7fff) / SYS_SCREEN_W,
                     0x4000 - (pm->y * 0x7fff) / SYS_SCREEN_H, 0, 0x7fff);
      break;
    case SDL_QUIT:
      QuitHit = 1;
      break;
    }
  }

  SysKeyState = SDL_GetKeyboardState(NULL);
  QuitHit |= (SysKeyState[SDL_SCANCODE_LALT] && SysKeyState[SDL_SCANCODE_Q]);
  QuitHit |= SysKeyState[SDL_SCANCODE_ESCAPE];
  static SysU32 Flags = 0;
  static SysS32 CurrentTurn = 0;

  SysS32 t = SysKeyState[SDL_SCANCODE_Q] - SysKeyState[SDL_SCANCODE_W];
  SysS32 f = SysKeyState[SDL_SCANCODE_A] - SysKeyState[SDL_SCANCODE_2];
  if ((t + (f << 8)) != CurrentTurn) {
    SysUserInput(SYSINPUT_ACCEL, SysNSec(), 0xff, t * 0x30, f * 0x30, 0x7fff);
    CurrentTurn = (t + (f << 8));
  }

  if (SysPause == 0)
    if (SysKeyState[SDL_SCANCODE_TAB])
      SysPause = 1;
  if (SysPause == 1)
    if (!SysKeyState[SDL_SCANCODE_TAB])
      SysPause = 4 | 1;
  if (SysPause == (4 | 1))
    if (SysKeyState[SDL_SCANCODE_TAB])
      SysPause = 2;
  if (SysPause == 2)
    if (!SysKeyState[SDL_SCANCODE_TAB])
      SysPause = 4 | 2;

  if (SysKeyState[SDL_SCANCODE_O]) {
    if (!(Flags & 4))
      SysUserInput(SYSINPUT_TOUCHON, 1, 0x4000, 0x4000 - 0x2000, 0, 0x7fff);
    Flags |= 4;
  } else if (Flags & 4) {
    SysUserInput(SYSINPUT_TOUCHOFF, 1, 0x4000, 0x4000 - 0x2000, 0, 0x7fff);
    Flags &= ~4;
  }

  if (SysKeyState[SDL_SCANCODE_P]) {
    if (!(Flags & 8))
      SysUserInput(SYSINPUT_TOUCHON, 2, 0x4000, 0x4000 + 0x2000, 0, 0x7fff);
    Flags |= 8;
  } else if (Flags & 8) {
    SysUserInput(SYSINPUT_TOUCHOFF, 2, 0x4000, 0x4000 + 0x2000, 0, 0x7fff);
    Flags &= ~8;
  }

  if (SysKeyState[SDL_SCANCODE_F]) {
    if (!(Flags & 16))
      SysUserInput(SYSINPUT_TOUCHON, 1, 0x4000 + 0x2000, 0x4000, 0, 0x7fff);
    Flags |= 16;
  } else if (Flags & 16) {
    SysUserInput(SYSINPUT_TOUCHOFF, 1, 0x4000 + 0x2000, 0x4000, 0, 0x7fff);
    Flags &= ~16;
  }

  if (SysKeyState[SDL_SCANCODE_SPACE]) {
    if (!(Flags & 32))
      SysUserInput(SYSINPUT_TOUCHON, 1, 0x4000 - 0x2000, 0x4000, 0, 0x7fff);
    Flags |= 32;
  } else if (Flags & 32) {
    SysUserInput(SYSINPUT_TOUCHOFF, 1, 0x4000 - 0x2000, 0x4000, 0, 0x7fff);
    Flags &= ~32;
  }

  return QuitHit;
}

SysS32 SysSwap(void) {
  static SysU32 FrameCount = 0, FPSFrameCount = 0,
                FPSTickCount = SDL_GetTicks();
  SysF32 FPS = 0;
  // SDL_GL_SwapBuffers( );
  SDL_GL_SwapWindow(SysMainWindow);
  FrameCount++;
  if ((SDL_GetTicks() - FPSTickCount) >= 1000) {
    FPS = ((FrameCount - FPSFrameCount) * 1000.0f) /
          (SDL_GetTicks() - FPSTickCount);
    FPSFrameCount = FrameCount;
    FPSTickCount = SDL_GetTicks();
    char r[256];
    sprintf(r, "(%3f FPS)", FPS);
    SDL_SetWindowTitle(SysMainWindow, r);
  }

  return SysPoll();
}

void SysAudioCallBack(void *UserData, SysU8 *Stream, SysS32 Len) {
  SysUserAudio(SysAudioFlags, SysAudioFreq, Stream, Len);
}

void DisplayTimeNSec(SysU64 t) {
  SysF64 f = fmod(t, 1000000000.0f);
  t /= 1000000000;
  SysU32 s = t % 60;
  t /= 60;
  SysU32 m = t % 60;
  t /= 60;
  SysU32 h = t % 24;
  t /= 24;
  SysU32 d = t;
  SysODS("%d:%d:%d:%d:%f", d, h, m, s, f);
}

static SysS32 SysOpened = 0;

void SysSDLExit(const char *msg) {
  printf("%s: %s\n", msg, SDL_GetError());
  SDL_Quit();
  exit(1);
}

void SysCheckSDLError(int line = -1) {
  const char *error = SDL_GetError();
  if (*error != '\0') {
    printf("SDL Error: %s\n", error);
    if (line != -1)
      printf(" + line: %i\n", line);
    SDL_ClearError();
  }
}

SysS32 SysOpen(SysS32 Flags) {
  //SysC8 *WDir = get_current_dir_name();
  SysODS("SDL Opening...\n");
  if (SDL_Init(SDL_INIT_VIDEO) < 0) /* Initialize SDL's Video subsystem */
    SysSDLExit("Unable to initialize SDL"); /* Or die on error */

  /* Request opengl 3.2 context.
   * SDL doesn't have the ability to choose which profile at this time of
   * writing, but it should default to the core profile */
  // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  /* Turn on double buffering with a 24bit Z buffer.
   * You may need to change this to 16 or 32 for your system */
  SysODS("Checking Render Buffers...\n");
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  SysODS("Opening Window...\n");
  /* Create our window centered at 512x512 resolution */
  SysMainWindow = SDL_CreateWindow(
      "V3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SYS_SCREEN_W,
      SYS_SCREEN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (!SysMainWindow) /* Die if creation failed */
    SysSDLExit("Unable to create window");

  SysODS("Setting Window Full Screen...\n");
  SDL_SetWindowFullscreen(SysMainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);

  SysODS("Checking Display Mode...\n");
  SDL_DisplayMode mode;
  SDL_GetWindowDisplayMode(SysMainWindow, &mode);
  SYS_SCREEN_W = mode.w;
  SYS_SCREEN_H = mode.h;

  //SysCheckSDLError(__LINE__);

  SysODS("Setting attribute sizes...\n");
  /* Create our opengl context and attach it to our window */
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SysODS("Creating context...\n");
  SysMainContext = SDL_GL_CreateContext(SysMainWindow);
  //SysCheckSDLError(__LINE__);

  /* This makes our buffer swap syncronized with the monitor's vertical refresh
   */
  SysODS("Setting swap interval...\n");
  SDL_GL_SetSwapInterval(1);

  SysODS("Getting RGBAZD attributes...\n");
  SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &GLRGBAZD[0]);
  SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &GLRGBAZD[1]);
  SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &GLRGBAZD[2]);
  SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &GLRGBAZD[3]);
  SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &GLRGBAZD[4]);
  SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &GLRGBAZD[5]);
  SysODS("Video Mode Set: RGBA:%d%d%d%d Z:%d DoubleBuffer:%d\n", GLRGBAZD[0],
         GLRGBAZD[1], GLRGBAZD[2], GLRGBAZD[3], GLRGBAZD[4], GLRGBAZD[5]);
  /*
      SDL_memset(&AudioFmt, 0, sizeof(AudioFmt));
      AudioFmt.freq = SysAudioFreq;
      AudioFmt.format = AUDIO_S16LSB;
      AudioFmt.channels = (SysAudioFlags&SYSAUDIO_STEREO)?2:1;
      AudioFmt.samples = SysAudioFreq/30;
      AudioFmt.callback = SysAudioCallBack;
      AudioFmt.userdata = NULL;
      if (SDL_OpenAudio(&AudioFmt, NULL) != 0)
      {
          printf("Audio Error!\n");
          exit(1);
      }
      */
  UDPOpen();
  SysODS("Performance Counter:%d Frequency:%d(%f)\n",
         SDL_GetPerformanceCounter(), SDL_GetPerformanceFrequency(),
         SDL_GetPerformanceFrequency() / 1000000000.0f);
  SysODS("SysOpen RealTime:");
  DisplayTimeNSec(SysNSec());
  SysODS(" Monotonic:");
  DisplayTimeNSec(SysNSec());
  SysODS("\n");
  SysOpened = 1;
  return 0;
}

SysS32 SysClose(SysS32 Flags) {
  SysAssert(SysOpened);
  UDPClose();
  SDL_GL_DeleteContext(SysMainContext);
  SDL_DestroyWindow(SysMainWindow);
  SDL_Quit();
  SysOpened = 0;
  return 0;
}

SysU32 SysState(SysS32 Cmd, void *Arg) {
  switch (Cmd) {
  case SYSSTATE_AUDIOOFF:
    SDL_PauseAudio(1);
    break;
  case SYSSTATE_AUDIOON:
    SDL_PauseAudio(0);
    break;
  case SYSSTATE_UDPON:
    break;
  }
  return 0;
}

//#define SYSIO
SysS32 SysLoad(SysU32 Flags, const SysC8 *OFileName, SysU32 Offset, SysU32 Size,
               void *Buffer) {
#ifdef SYSIO

  FILE *f;
  SysS32 l;

  SysC8 FileName[256];
#ifdef SYSLSDIR
  if (Flags == SYSLOADSAVE_USERDIR) {
    strcpy(FileName, "../");
    strcat(FileName, OFileName);
  } else
#endif
    strcpy(FileName, OFileName);

  f = fopen(FileName, "rb");
  if (f == NULL) {
    return 0;
  }
  if (Size && Buffer) {
    fseek(f, Offset, SEEK_SET);
    l = fread(Buffer, 1, Size, f);
  } else {
    fseek(f, 0, SEEK_END);
    l = ftell(f);
  }
  fclose(f);
  return l;
#else
  return 0;
#endif
}

SysS32 SysSave(SysU32 Flags, const SysC8 *OFileName, SysU32 Size,
               void *Buffer) {
#ifdef SYSIO
  SysC8 FileName[256];
#ifdef SYSLSDIR
  if (Flags == SYSLOADSAVE_USERDIR) {
    strcpy(FileName, "../");
    strcat(FileName, OFileName);
  } else
#endif
    strcpy(FileName, OFileName);
  FILE *f = fopen(FileName, "wb");
  SysAssert(f);
  if (Size)
    fwrite(Buffer, sizeof(char), Size, f);
  fclose(f);
#endif
  return 0;
}

SysU64 SysNSec(void) {
    SysF64 cf = SysF64(SDL_GetPerformanceCounter())*1000000000.0f/SysF64(SDL_GetPerformanceFrequency());
    return SysU64(cf);
}

static SysU8 M32Hash(void *m) {
  SysU32 a = (SysU64)m;
  a = ((((a ^ (a >> 8)) + (a >> 16)) ^ (a >> 24)) & 0xff);
  if (!a)
    return 0xff;
  else
    return a;
}

void *SysAllocate(SysU32 Bytes) {
  SysU32 Align = 16;
  SysU8 *m = (SysU8 *)malloc(Bytes + Align);
  SysU8 h = M32Hash(m);
  *m++ = h;
  while (((SysU64)m) & (Align - 1))
    *m++ = 0;
  return m;
}

SysU32 SysDeAllocate(void *Mem) {
  if (!Mem)
    return 0;
  SysU8 *m = (SysU8 *)Mem;
  do {
    --m;
  } while (!m[0]);
  SysAssert(m[0] == M32Hash(m));
  free(m);
  return 0;
}

#ifdef SYS_DEBUG_ODS
void SysODS(const SysC8 *DebugString, ...) {
  va_list args;
  va_start(args, DebugString);
  vprintf(DebugString, args);
  va_end(args);
}
#endif
SysS32 SysR = 0;
void SysLoop(void) {
  SysU32 s = 0;
  if (SysPause == (4 | 2)) {
    SDL_PauseAudio(0);
    s = SYSFRAME_RESUME;
    SysPause = 0;
  } else if (SysPause) {
    s = SYSFRAME_PAUSE;
    SDL_PauseAudio(1);
  }
  if (SysActive == 1)
    s = SYSFRAME_PAUSE;
  else if (SysActive) {
    s = SYSFRAME_RESUME;
    SysActive = 0;
  }
  SysR = SysUserFrame(s);
  if (SysSwap())
    SysR = SYSFRAME_QUIT;
}

int main(int argc, char *argv[]) {
  SysR = 0;
  SysOpen(0);
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(SysLoop, 0, true);
#else
  while (SysR != SYSFRAME_QUIT)
    SysLoop();
#endif
  SysClose(0);
  SysODS("Sys Finished\n");
  return 0;
}
