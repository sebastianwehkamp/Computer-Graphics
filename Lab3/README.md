# Assignment 3

The program implements assignment 3. There were 2 main assignments:

1. Project vertices (at any subdivision level) to their limit positions using limit stencils. Boundaries should be supported!:
The main part of the code for this assignment is located in 'meshtools.cpp' in the function computeLimitMesh(). Which approximates subdvision surfaces with Greory patches for hardware tesselation. 

2. Render surface patches corresponding to the regular quads in the control net using tesselaton shaders. 
The tesselation shaders themselves are located in tcs.glsl and tes.glsl which render the surface patches.

Three images are attached, the first image "normalSuzanne.png" is an image showing the default usage of the program. Compared to "limitSuzanne.png" which uses the limit you can clearly see the difference. "TesselationSuzanne.png" shows the results of assignment 2. 
