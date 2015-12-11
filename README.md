# GPU cloth with OpenGL Compute Shaders

This project in progress is a PBD cloth simulation accelerated and parallelized using OpenGL compute shaders. For more details on the project motivation and milestones, see the [slides](https://docs.google.com/presentation/d/1t7K-MafQH_8fw7R2KPMy-iOtFBdPRJSsHp4oSA9XR0c/edit?usp=sharing).

## Milestone 3 - 12/7/2015
[slides](https://docs.google.com/presentation/d/1OpCrZfxQcJsMGToeXmzwc1cdoZwTzvEy3AgFFTF1_hQ/edit?usp=sharing)

This week's tasks included:
- working on simulation stability
- getting collision detection and response started

### Notes
- one big reason for the instability last week was that one of my internal constraint buffers was getting filled with bogus values
- I was filling another buffer with bogus values after filling the internal constraint buffers, but I forgot to bind the "bogus buffer"
- so almost all my points were missing a whole constraint out of the 4 available at this time
- there is still an odd problem with coherence, though: using only shader stages that cause gravitational acceleration, a cloth in the X/Y plane (z-up world) quickly transforms into this instead of falling flat:
![](images/incoherent_cloth.png)

## Milestone 2 - 11/30/2015
[slides](https://docs.google.com/presentation/d/1S4NgARMeADFadHxYxQ_WwaJtJeHCqSJZSSER5r8drzc/edit?usp=sharing)

This week's tasks included resolving many of the problems in the previous week, namely:
- the problem with things in a single plane ending up in 3 planes
- improving rendering to make this more debuggable
- nailing bugs in the simulation pipeline

### Notes
- On the recommendation of [Harmony Li](https://github.com/harmoli) I switched the simulator to use PBD. There are still considerable stability problems
- Compute shaders expect shader storage buffers to act like vec4 buffers when data is being transferred, so transferring the positions as vec4s fixed the "planes" problem from last week

## Milestone 1 - 11/23/2015
[slides](https://docs.google.com/presentation/d/1CzXv1JUJpYXYMGLLSd8CTBg-UIMnl0YaXnWc5PCvoLs/edit?usp=sharing)

This week was mostly spent on reading up on compute shaders (found a lot of similarities with CUDA) and exploring/fixing the base code. I also put together a naive cloth simulation based on explicit symplectic integration, but it still needs considerable debugging.

### Notes
- for some reason, going from position buffers -> the vertex shader, many vertices seem to have their coordinate system modified to use a different axis as up. My hunch is there's something with the geometry shader
- in this example, all the vertex positions are only in the X/Z plane:
![](images/milestone_1_geometry_shader_bug.png)

- compute shader atomics are only for integers, which could prove to be a problem
- it's hard to tell exactly how "wrong" the simulation is at the moment because of the issue in part 1
- this is a case in which slightly better rendering would vastly improve productivity in debugging

### added to the next milestone:
- rendering improvements: figure out how to render a cloth mesh instead of points, ground plane, lambert shading
- figure out what is happening with the geometry shader (or if that's even the problem)
- assess the simulation stability/"correctness"

### Done
- most of the "parts" of the simulation pipeline are in place
- added some camera controls to help with debugging