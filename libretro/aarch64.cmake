# Cross toolchain for the plorpOS Brick (tortos-toolchain image, glibc 2.17 symbols).
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
