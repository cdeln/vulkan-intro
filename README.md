# Introduction to Vulkan

## Background

This is an introduction to the Vulkan core API.
The introduction targets an audience without previous knowledge of Vulkan, thoughg background in OpenGL or some other graphics framework is preferable.
For a reader belonging to this audience, Vulkan can be a bit intimidating at first glance.
The offical specification (*the specs*) (https://registry.khronos.org/vulkan/specs/1.3-khr-extensions/html/)
and the official tutorial (https://vulkan-tutorial.com/) are quite dense.
In order to learn Vulkan you will need to find resources on the right level for you.
At the time of writing this document I am a beginner myself, and my intention is that this will aid others in learning Vulkan as well.
Below is a list of good reading material (should be read in that order).

- Read up on how to learn Vulkan: https://www.jeremyong.com/c++/vulkan/graphics/rendering/2018/03/26/how-to-learn-vulkan/
- Step through the Vulkan Guide: https://github.com/KhronosGroup/Vulkan-Guide

Then you are ready to read through the code here.
After reading through this the code here, I recommend reading through the classic tutorials to actually render something to screen

- The Intel tutorial (skip the dynamic library stuff): https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-1.html
- The official tutorial: https://vulkan-tutorial.com/

Certain concepts in Vulkan are not well described in tutorials but are really well described in the specification.
The specification is extremely dense, and is hard to read because it is aimed towards both implementors of the Vulkan API and its users at the same time.
The specification aims to be non-ambigiuous, not necessarily easy to understand.
With that said, I believe the following sections are good to read to properly understand Vulkan (after reading through tutorials ofc)

- Execution and Memory Dependencies: https://registry.khronos.org/vulkan/specs/1.0/html/vkspec.html#synchronization-dependencies

I highly recommend to tag along and write the code yourself, no copy pasting.
Make sure you have a good setup with and editor and autocompletion, or your fingers will hate you.
I personally use Vim with CoC plugin for autocompletion using Clangd 15.
Spin up a web browser with some documentation

- Official 1.3 specs: https://registry.khronos.org/vulkan/specs/1.3/html
- Dark mode 1.0 specs: https://devdocs.io/vulkan

Install the SDK from tarball.
I initially used the official Vulkan packages that comes with my distro (Ubuntu 20.04 LTS), but the validation support was bad.

Also check out these tools (I haven't got the chance to use them myself yet)

- https://renderdoc.org/
- https://gapid.dev/

Some notes about the Vulkan API

- The API makes almost no assumption about its usage.
  This is the reason why so much code is required for so little action.
- The API is decoupled. Almost every entity can be created with minimal information about other entities.
  For example, a framebuffer can be created independently of a render pass.
  A render pass describes dependencies between attachments in render sub passes, and a framebuffer describes which images should be used as attachments.
  Hence, different framebuffers can be used with a single render pass, and different render passes can be used with a single framebuffer etc.
  This makes us write very similar code over and over again, and that code needs to be compatible.
  This might seem very error prone, but validation layers helps alot!

## Usage

You need Vulkan SDK installed and CMake.
If you don't want to bother installing the SDK and you use Ubuntu you can quickly install Vulkan using `scripts/setup` (discouraged for reasons described in background).

Configure and build the code

    ./scripts/configure Debug
    ./scripts/build Debug

Run it

    ./out/Debug/main

Look at the result

    cat out.bmp
