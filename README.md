# Self-contained 3dfx Voodoo 1 (SST-1) demos

## What is this?

This repository demonstrates the internal workings of the 3dfx Voodoo 1, the revolutionary 3D accelerator that transformed PC gaming in 1997 and launched the modern 3D gaming industry.

I've integrated the Dosbox-X Voodoo emulator with components of the original 3dfx Glide graphics API to create several small demos that run natively on modern (2025) Mac and Linux systems.

![](doc/voodoo_demos.drawio.svg)

## Gallery

triangle.cpp: Basic triangle rendering

![Triangle Demo](doc/triangle.png)

cube.cpp: A smooth rotating cube

![Cube Demo](doc/cube.png)

teapot.cpp: The classic Utah teapot with lighting effects

texcube.cpp: A texture-mapped rotating cube

## Acknowledgements
* Voodoo emulator by [Aaron Giles](https://aarongiles.com/programming/war-mame/).
* [Glide API](https://github.com/Danaozhong/3dfx-Glide-API) open sourced by 3dfx.