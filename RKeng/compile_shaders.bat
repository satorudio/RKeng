@echo off
REM Компилируем шейдеры в SPIR-V
REM glslc идёт в комплекте с Vulkan SDK

set GLSLC=%VULKAN_SDK%\Bin\glslc.exe

if not exist "%GLSLC%" (
    echo ERROR: glslc not found at %GLSLC%
    echo Make sure Vulkan SDK is installed and VULKAN_SDK env var is set.
    pause
    exit /b 1
)

echo Compiling shaders...

"%GLSLC%" shaders/triangle.vert -o shaders/triangle.vert.spv
"%GLSLC%" shaders/triangle.frag -o shaders/triangle.frag.spv

echo Done. SPV files created in shaders/
pause
