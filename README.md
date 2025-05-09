# Self-contained 3dfx Voodoo 1 (SST-1) demos

## What is this?

This repository demonstrates the internal workings of the 3dfx Voodoo 1, the revolutionary 3D accelerator that transformed PC gaming in 1996 and launched the modern 3D gaming industry.

I've integrated the Dosbox-X Voodoo emulator with components of the original 3dfx Glide graphics API to create several small demos that run natively on modern (2025) Mac and Linux systems. You can now program using the Glide API as if it were 1997, or even debug and step through the emulator to explore how the rasterization hardware works.

![](doc/voodoo_demos.drawio.svg)

## Running

The only dependency is SDL. To install SDL, use `brew install sdl` on Mac, or `sudo apt install libsdl2-dev` on Linux  or WSL on Windows.

Then use `make` to build all the demos.
```
cd 3dfx_voodoo_demos
make
```

## Gallery

triangle.cpp: Basic triangle rendering

![Triangle Demo](doc/triangle.png)

cube.cpp: A smooth rotating cube

![Cube Demo](doc/cube.png)

teapot.cpp: The classic Utah teapot with Gouraud lighting

https://github.com/user-attachments/assets/797c2e0a-169e-4c22-a931-1dc4ed47a739

texcube.cpp: A texture-mapped rotating cube

https://github.com/user-attachments/assets/f94c5fbd-1c50-4d5f-92eb-46100423361e


## Acknowledgements
* Voodoo emulator by [Aaron Giles](https://aarongiles.com/programming/war-mame/).
* [Glide API](https://github.com/Danaozhong/3dfx-Glide-API) open sourced by 3dfx.
