~/emsdk/emscripten/1.38.11/em++ -v -g4 -s FULL_ES2=1 -s USE_SDL=2 -DSYS_DEBUG_ODS V3D.cpp GFX.cpp  Strings.cpp SysSDL.cpp -o ~/werelien.github.io/v3d.html 
mv ~/werelien.github.io/v3d.html ~/werelien.github.io/index.html
