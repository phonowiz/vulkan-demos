# Mac OS Vulkan Renderer

This is my test bed for all things vulkan.  It is project using MoltenVK on a mac and a setup using Xcode.  Vulkan projects are not very common on the mac and I would like to set up an example for others to learn from.

Purpose
----------
I want to create renderer that I could use to prototype ideas quickly, game engines are very good for this, but I'd rather implement everything from scratch because you learn more that way.  The need came about because Apple will deprecate OpenGL soon, and I need to port my other project to Vulkan, you can check it out here: https://github.com/phonowiz/voxel-cone-tracing

I ended up implementing a frame graph (or render graph) based off of the chat which can be found here:

https://www.ea.com/frostbite/news/framegraph-extensible-rendering-architecture-in-frostbite

There are other implementations of this topic on the internet.  But I think they are too low level and as result complex to use for my purposes. 

My philosophy is to make something that satisfies my needs specifically and only expose exactly what I need from Vulkan, the rest can stay hidden with default values. As my development gets more sophisticated, I'll keep exposing more and more of the API, just enough to get what I need done.  It keeps things simpler. 


Tutorial
-----------

### Concepts

Fist, some concepts to introduce.  At a very high level, the **frame graph** (or **render graph**) is a structure which has global knowledge of what is being rendered; it knows about the relationships between render passes and and their dependecies on other render passes.  This is not to be confused with a scene graph, which is another structure with knowledge of entities (like meshes for example) in a game scene.  It is important that a frame graph is acyclic, otherwise you can have 2 render passes which depend on each other.  Render passes are described next.

[**Render passes**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/render_pass.h) are a way to tie all Vulkan (as well my API) concepts together into one unit.  All the stuctures, in one way or another, are tied to a render pass.  Render passes are composed of **subpasses**, which accomplish a specific task in a series tasks needed to render something. A render pass may have one or more subpasses.

Each subpass owns a **pipeline**. Pipelines control different rendering states and cannot be changed once created.  There are two types of pipelines, [**graphics pipelines**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/pipelines/graphics_pipeline.h) and [**compute pipelines**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/pipelines/compute_pipeline.h).  Graphics Pipelines are used to communicate with visual materials, compute pipelines are used to communicate with compute materias. These are described next. 

**Materials** take care of creating shaders and handling parameters that shaders need, as well as passing values to the GPU to be used for when rendering occurs.  At the moment of this writing, I only support one uniform buffer, one dynamic buffer but no API restrictions on how many samplers your shader uses.  This is not a Vulkan limitation, more of a limitation I put on myself for implementation simplicity.  This applies to both vertex, fragment, and compute shaders.  Uniform and dynamic buffers use std140 memory layout, so make sure to write so in your shader uniform and dynamic buffers.  There are two types of materials, [**visual materials**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/materials/visual_material.h), used with graphics pipelines, and [**compute materials**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/materials/compute_material.h), used with compute pipelines.

Finally, to build a render graph, you have to create [**graph**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/graph.h) object which contains one more [**nodes**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/node.h).  Nodes themselves can have other nodes attached to it. Each node is in charge of executing a render pass.  The code works by doing a depth first traversal (post order) of the graph.  The children always do their tasks first, and then the parents.  This gives the parent a chance to have all its dependencies created and ready to be used for when the parent itself starts executing commands.  Each node gets a chance to **init**, **record**, and **execute**.  Init only needs to happen once, recording and executing happens on a per frame basis.  And lastly destruction, which happens at the very end.  

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

