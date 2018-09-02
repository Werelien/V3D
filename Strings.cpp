#include "Strings.hpp"
#include "GFX.hpp"
const SysC8 *STR3VSrc[] = {
    
    "attribute vec3 V;\n"
    "attribute vec4 T;\n"
    "attribute vec4 C;\n"
    "uniform mat4 M[16];\n"
    "uniform vec4 A[20];\n"
    "varying vec4 vcol;\n"
    "varying vec4 vtex;\n"
    "void main()\n"
    "{\n"
    "    vec4 v=vec4(V,1.0);\n"
    "    vec4 p=M[0]*(v+A[1]*T);\n"
    "    vcol=A[0]*C/255.0;\n"
    "    gl_Position=p;\n"
    "    vtex=T;\n"
    "}\n",
#ifdef __EMSCRIPTEN__
    "precision mediump float;\n"
#endif
    "uniform sampler2D U[2];\n"
    "uniform vec4 F[2];\n"
    "varying vec4 vcol;\n"
    "varying vec4 vtex;\n"
    "void main()\n"
    "{\n"
    "   vec4 color=texture2D(U[0],vec2(vtex));\n"
    "   gl_FragColor=vcol*color;\n"
    "}\n",
    0
};
