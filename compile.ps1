# Now using the CLANG64 environment!

# run this in msys2 terminal: 
# pacman -S mingw-w64-clang-x86_64-clang
# pacman -S mingw-w64-ucrt-x86_64-compiler-rt
# pacman -S mingw-w64-clang-x86_64-raylib

$outFile = "app.exe"

$compiler = "C:\msys64\clang64\bin\clang++.exe"
$lib = "./lib/"

Invoke-Expression "$compiler main.cpp -o $outFile -O1 -Wall -g -I ./include/ -L $lib -fsanitize=undefined -fsanitize=address -lraylib -lopengl32 -lgdi32 -lwinmm";

Invoke-Expression "cp $outFile ./TryingToRun/"

# Old:
# Invoke-Expression "$compiler main.cpp -o $outFile -O1 -Wall -g -I ./include/ -L ./lib/ -lraylib -lopengl32 -lgdi32 -lwinmm";