glslc -fshader-stage=vertex -fentry-point="vs_main" hlsl/color.hlsl -o out/color.vert.spv -fresource-set-binding b0 0 0 b1 1 0 b2 2 0 b3 3 0 t0 1 1 t1 1 2 t2 1 3 -fauto-combined-image-sampler
glslc -fshader-stage=fragment -fentry-point="ps_main" hlsl/color.hlsl -o out/color.frag.spv -fresource-set-binding b0 0 0 b1 1 0 b2 2 0 b3 3 0 t0 1 1 t1 1 2 t2 1 3 -fauto-combined-image-sampler

glslc -fshader-stage=vertex -fentry-point="vs_main" hlsl/edge.hlsl -o out/edge.vert.spv -fresource-set-binding b0 0 0 b1 1 0
glslc -fshader-stage=fragment -fentry-point="ps_main" hlsl/edge.hlsl -o out/edge.frag.spv -fresource-set-binding b0 0 0 b1 1 0

copy .\hlsl out

pause