# ns-box

## Requirements

-   CMake
-   C++ compiler with C++17 support (gcc or clang)
-   Ninja or Make

## Build Instructions

-   Initially create a build folder in project root with `mkdir build`.
-   `cd build`
-   `cmake ..` (or `cmake -G Ninja ..` if you want to use ninja instead of make)
-   `make` or `ninja` depending on what got generated
-   `make clang-format-project-files` or `ninja clang-format-project-files` to
    run clang-format on all the files
