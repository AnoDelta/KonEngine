@echo off
:: Build and run the KonEngine test suite
if not exist build mkdir build
cmake -B build -DKON_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target KonEngineTest
echo.
echo Running tests...
echo.
build\tests\KonEngineTest.exe
pause
