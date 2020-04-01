# Mac OS Vulkan Renderer

This is my test bed for all things vulkan.  It is project using MoltenVK on a mac and a setup using Xcode.  Vulkan projects are not very common on the mac and I would like to set up an example for others to learn from.

Purpose
----------
I want to create renderer that I could use to prototype ideas quickly, game engines are very good for this, but I'd rather implement everything from scratch because you learn more that way.  The need came about because Apple will deprecate OpenGL soon, and I need to port my other project to Vulkan, you can check it out here: https://github.com/phonowiz/voxel-cone-tracing

This code is not very organized as of now, there is still lots of code moving all over the place as I try what I think is right.  The meaningful work can be found in the folder "vulkan_wrapper" which you can look at here: https://github.com/phonowiz/vulkan-gui-test/tree/master/vulkan-demos.  


My philosophy is to make something that satisfies my needs specifically and only expose exactly what I need from Vulkan, the rest can stay hidden with default values. As my development gets more sophisticated, I'll keep exposing more and more of the API, just enough to get what I need done.  It keeps things simpler. 

## Deferred Rendering
Here are screenshots of my deferred renderings, these will be used for voxel cone tracing. 

#### Albedo
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/albedo.png">

#### Depth
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/depth.png">

#### Direct Lighting
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/direct-lighting.png">

#### Normals
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/normals.png">

#### 3D Texture Visualization
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/3d-texture visualization.png">
The streaks you see here are as result of the ray marching am doing to render the 3D texture.  This view is mostly for debugging purposes and designed to show me what the 3D texture content is, how I visualize this doesn't contribute to the final cone traced image, but the contents of the 3D texture will contrinbute to final cone traced image.   

#### Ambient Occlusion
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/ambient_occlusion.png">

#### Direct+Ambient Lighting with Ambient Occlusion
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/ambient+direct.png">
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/ambient+direct+backlit.png">

