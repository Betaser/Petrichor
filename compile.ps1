# Now using the CLANG64 environment!

# run this in msys2 terminal: 
# pacman -S mingw-w64-clang-x86_64-clang
# pacman -S mingw-w64-ucrt-x86_64-compiler-rt
# pacman -S mingw-w64-clang-x86_64-raylib
param(
    [ValidateSet("run", "build")]
    [string] $mode = "run",
    [ValidateSet("true", "false")]
    [string] $staticLinking = "false"
)

$runMode = "run"
$buildMode = "build"

Write-Output "mode = $mode, staticLinking = $staticLinking"

$compiler = "C:\msys64\clang64\bin\clang++.exe"

# Static linking doesn't support fsanitize debugging
if ($staticLinking -eq "true") {
    $lib = "./lib/"
    $outFile = "./app.exe"
    Invoke-Expression "$compiler main.cpp -o $outFile -O1 -Wall -Wextra -Werror -pedantic -std=c++20 -DRAYLIB_STATIC -DGRAPHICS_API_OPENGL_33 -static -I ./include/ -L $lib -lraylib -lopengl32 -lgdi32 -lwinmm"
} else {
    $lib = "./lib/dynamic/"
    $outFile = "./TryingToRun/app.exe"
    Copy-Item -Path "./assets/*" -Destination "./TryingToRun/assets" -Recurse
    Invoke-Expression "$compiler main.cpp -o $outFile -O1 -Wall -Wextra -Werror -pedantic -std=c++20 -g -fsanitize=undefined -fsanitize=address -I ./include/ -L $lib -lraylib -lopengl32 -lgdi32 -lwinmm"
}

switch ($mode) {
    $runMode {
        if (0 -eq $LastExitCode) {
            Invoke-Expression $outFile
        } 
        # Compilation did not work
        else {
            Write-Output "Last exit code failed: ${LastExitCode}, see error above"
        }
    }
    $buildMode {
        Write-Output "Wrote to file $outFile"
        if (0 -ne $LastExitCode) {
            Write-Output "Last exit code failed: ${LastExitCode}, see error above"
        }
    }
}

# Old:
# Invoke-Expression "$compiler main.cpp -o $outFile -O1 -Wall -g -I ./include/ -L ./lib/ -lraylib -lopengl32 -lgdi32 -lwinmm";