~/emsdk/emscripten/1.38.11/em++ -v -O3 -s FULL_ES2=1 -s USE_SDL=2  V3D.cpp GFX.cpp  Strings.cpp SysSDL.cpp -o ~/webv3d/v3d.html 
mv ~/webv3d/v3d.html ~/webv3d/index.html
