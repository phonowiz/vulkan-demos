# Mac OS Vulkan Renderer

This is my test bed for all things vulkan.  It is project using MoltenVK on a mac and a setup using Xcode.  Vulkan projects are not very common on the mac and I would like to set up an example for others to learn from.

## Purpose

I want to create renderer that I could use to prototype ideas quickly, game engines are very good for this, but I'd rather implement everything from scratch because you learn more that way.  The need came about because Apple will deprecate OpenGL soon, and I need to port my other project to Vulkan, you can check it out here: https://github.com/phonowiz/voxel-cone-tracing

I ended up implementing a frame graph (or render graph) based off of the chat which can be found here:

https://www.ea.com/frostbite/news/framegraph-extensible-rendering-architecture-in-frostbite

There are other implementations of this topic on the internet.  But I think they are too low level and as result complex to use for my purposes. 

My philosophy is to make something that satisfies my needs specifically and only expose exactly what I need from Vulkan, the rest can stay hidden with default values. As my development gets more sophisticated, I'll keep exposing more and more of the API, just enough to get what I need done.  It keeps things simpler. 


## Concepts


Fist, some concepts to introduce.  At a very high level, the **frame graph** (or **render graph**) is a structure which has global knowledge of what is being rendered; it knows about the relationships between render passes and and their dependecies on other render passes.  This is not to be confused with a scene graph, which is another structure with knowledge of entities (like meshes for example) in a game scene.  It is important that a frame graph is acyclic, otherwise you can have 2 render passes which depend on each other.

[**Render passes**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/render_pass.h) are a way to tie all Vulkan (as well my API) concepts together into one unit.  All the stuctures, in one way or another, are tied to a render pass.  Render passes are composed of **subpasses**, which accomplish a specific task in a series tasks needed to render something. A render pass may have one or more subpasses.

Each subpass owns a **pipeline**. Pipelines control different rendering states and cannot be changed once created.  There are two types of pipelines, [**graphics pipelines**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/pipelines/graphics_pipeline.h) and [**compute pipelines**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/pipelines/compute_pipeline.h).  Graphics Pipelines are used to communicate with visual materials, compute pipelines are used to communicate with compute materias. These are described next. 

[**Materials**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/materials/material_base.cpp) take care of creating shaders and handling parameters that shaders need, as well as passing values to the GPU to be used for when rendering occurs.  At the moment of this writing, I only support one uniform buffer, one dynamic buffer but no API restrictions on how many samplers your shader uses.  These are not Vulkan limitations, more of limitations I put on myself for implementation simplicity.  These limitations apply to vertex, fragment, and compute shaders.  Uniform and dynamic buffers use std140 memory layout, so make sure to write so in your shader uniform and dynamic buffers.  There are two types of materials, [**visual materials**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/materials/visual_material.h), used with graphics pipelines, and [**compute materials**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/materials/compute_material.h), used with compute pipelines.

To build a render graph, you have to create [**graph**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/graph.h) object which contains one more [**nodes**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/node.h).  Nodes themselves can have other nodes attached to it. Each node is in charge of executing a render pass.  The code works by doing a depth first traversal (post order) of the graph.  The children always do their tasks first, and then the parents.  This gives the parent a chance to have all its dependencies created and ready to be used for when the parent itself starts executing commands.  Each node gets a chance to **init**, **record**, and **execute**.  Init only needs to happen once, recording and executing happens on a per frame basis.  And lastly destruction, which happens at the very end.

A few more concepts about render graphs in my API. [**Command recorder**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/command_recorder.h), like the name states, records commands.  As the graph is traversed in a post order fashion, the graph will use its command recorder to record commands from either [**visual nodes**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/graphics_node.h) or [**compute nodes**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/compute_node.h).  A **master node** will then grab the commands and render to a [**present textures**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/textures/glfw_present_texture.h), which are the textures you see on screen.  The root node is always a master node because it always renders to these textures.  Master nodes can be introduced as children in the graph as well, this is useful for debugging purposes, maybe you want to see what a texture looks like, in which case you'd use the [**display_texture_2d**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/graph_nodes/graphics_nodes/display_texture_2d.h).  You can tell this is master node because the ``record_node_commands`` function returns false, this tells the graph to stop recording and present what is found in the present textues.  Lastly, but not least, is the [**texture registry**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/render_graph/texture_registry.h), whose job is keep track of which texture assets a node depends on.  This becomes useful for when we are recording commands and we want to introduce barriers to make sure no one starts reading before an asset has been written to completely. 

Ok, I think I've laid the ground work to start looking at a very simple example which ties all of these concepts together.  Let's explore that next. 

## Example

### Graphics Node

The following is the function ```init``` from [**display_texture_2d**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/graph_nodes/graphics_nodes/display_texture_2d.h) node.

```c++
    virtual void init_node() override
    {
        EA_ASSERT_MSG(_texture != nullptr, "No texture was assigned to this node");
        
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        
        
        pass.get_attachment_group().add_attachment( _swapchain->present_textures, glm::vec4(0.0f));
        
        _screen_plane.create();
        subpass_type& sub_p = pass.add_subpass(parent_type::_material_store, _material);
        
        if(_texture_type == vk::render_texture::get_class_type())
        {
            vk::resource_set<vk::render_texture>& rsrc = _tex_registry->get_read_render_texture_set(_texture, this, vk::usage_type::COMBINED_IMAGE_SAMPLER);
            sub_p.set_image_sampler(rsrc, "tex", vk::parameter_stage::FRAGMENT, 1,
                                           vk::usage_type::COMBINED_IMAGE_SAMPLER);
        }
        else if(_texture_type == vk::depth_texture::get_class_type())
        {
            vk::resource_set<vk::depth_texture>& rsrc = _tex_registry->get_read_depth_texture_set(_texture, this, vk::usage_type::COMBINED_IMAGE_SAMPLER);
            sub_p.set_image_sampler(rsrc, "tex", vk::parameter_stage::FRAGMENT, 1,
            vk::usage_type::COMBINED_IMAGE_SAMPLER);
        }
        else if(vk::texture_2d::get_class_type() == _texture_type)
        {
            vk::texture_2d& rsrc = _tex_registry->get_loaded_texture(_texture, this, parent_type::_device, _texture);
            sub_p.set_image_sampler(rsrc, "tex", vk::parameter_stage::FRAGMENT, 1,
                                            vk::usage_type::COMBINED_IMAGE_SAMPLER);
        }
        else
        {
            EA_FAIL_MSG("unrecognized texture");
        }
        
        
        int binding = 0;
        sub_p.init_parameter("width", vk::parameter_stage::VERTEX,
                             _swapchain->get_vk_swap_extent().width, binding);
        sub_p.init_parameter("height", vk::parameter_stage::VERTEX,
                             _swapchain->get_vk_swap_extent().height, binding);
        
        sub_p.add_output_attachment("present");
        pass.add_object(_screen_plane);
        
    }

```

```init``` functions is where nodes decide what they need in order for their render passes to work properly.  Let's break this down from the top.  

The function

```C++
  pass.get_attachment_group().add_attachment( _swapchain->present_textures, glm::vec4(0.0f));
```
is saying that the render pass for this node is going to render the the swapchain present textures, the clear value to be used is zero for all channels.  A couple of new words to define here, whenever you see the word ***group*** or [**attachment group**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/textures/attachment_group.h) in the code, it refers to a group of [***resource sets***](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/textures/resource_set.h). In Vulkan, you don't have to wait for rendering to finish on the CPU side in order to continue performing operations, every frame can be rendered in parallel while the rendering is happening on the GPU side.  Resource set size depends on how many images your swapchain can handle.  On my version of MoltenVk is 3, therefore, all resource sets in the code are of size 3. So when this node executes, it renders to the first image in the resource set, then the cpu will not wait, it'll continue to render to the 2nd image in the resource set, and so forth.  [**_swapchain->present_textures**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/textures/glfw_present_texture.h) is a resource set of these textures whose job is to display things on screen.  In this particular example, we have an attachment group of size 1 ( as in 1 resource_set), whose resource_set is of size 3 (for the 3 images the swapchain can handle)

This line of code just creates a screen plane we can use to render the texture:
```C++
  _screen_plane.create();
```
This line of code creates a subpass and assigns to it a _material, which is just the name of a material in the [**material store**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/vulkan_wrapper/materials/material_store.cpp):

```C++
  subpass_type& sub_p = pass.add_subpass(parent_type::_material_store, _material);
```
Every subpass needs one material assigned to it, and this material will define how things get rendered.  If this were a compute pipeline, we'd use a compute material instead.

The ``if`` statement that follows in the code is just looking for what type of texture the client wants to display, but pretty much the concept is the same for each branch.  Let's look at one example:

```C++

            vk::resource_set<vk::render_texture>& rsrc = _tex_registry->get_read_render_texture_set(_texture, this, vk::usage_type::COMBINED_IMAGE_SAMPLER);
            sub_p.set_image_sampler(rsrc, "tex", vk::parameter_stage::FRAGMENT, 1,
                                           vk::usage_type::COMBINED_IMAGE_SAMPLER);

```

What this says is: look for the render texture set in the texture registry, and then assign this render texture set to the argument "tex" bound at 1 of the fragment shader of the material.  We plan to use this texture in the shader as a combined image sampler.  Look at the [**display_plane.frag**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/shaders/graphics/display_plane.frag).  All of this information becomes useful when deciding how we want the layout to be when the barrier gets created when recording commands.

The following lines of code will create arguments for the material to consume, the arguments **MUST** be created in the order in which they appear in the shader, in this case the shader defined is [**display_plane.vert**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/shaders/graphics/display_plane.vert), click on the link, you'll see how the binding argument matches the binding specified in the shader.

```C++
        int binding = 0;
        sub_p.init_parameter("width", vk::parameter_stage::VERTEX,
                             _swapchain->get_vk_swap_extent().width, binding);
        sub_p.init_parameter("height", vk::parameter_stage::VERTEX,
                             _swapchain->get_vk_swap_extent().height, binding);
```

Since this is a master node (a node which renders to present textures), then we must tell the graph not to record any more commands, and to just present what is found in present textues, here is how we do that:

```C++
    virtual bool record_node_commands(vk::command_recorder& buffer, uint32_t image_id) override
    {
        parent_type::record_node_commands(buffer, image_id);
        return false; //if this were true, then commands can continue to record, possibly stumping on whats on the present textures
    }
```
The last line of code simply adds our screen plane to the render pass.  This screen plane will go through the subpass we talked above, causing the texture to render to the present textures:

```C++
  pass.add_object(_screen_plane);
```

In [this](https://github.com/phonowiz/vulkan-demos/tree/master/vulkan-demos/graph_nodes/graphics_nodes) directory, you'll find many more  examples that will hopefully help you understand what else can be done.  By now, you have enough knowledge to give you a good sense of what these nodes are trying to accomplish. 


### Build A Graph

Here is an example of how to build graph based on the node above:

```C++

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    constexpr int DEFAULT_VSYNC = 1;
    glfwSwapInterval(DEFAULT_VSYNC);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    
    int width = 1024;
    int height = 768;
    
    GLFWwindow* window = glfwCreateWindow(width, height, "Sample Demo", nullptr, nullptr);
    VkSurfaceKHR surface;

    vk::device device;
    glfwCreateWindowSurface(device._instance, window, nullptr, &surface);
    
    vk::glfw_swapchain swapchain(&device, window, surface);
      
    device.create_logical_device(surface); 
    
    //material store is just the class which loads up all materials to be used by the nodes
    vk::material_store material_store;
    material_store.create(&device);
    
    //set up the swapchain 
    vk::glfw_swapchain swapchain(&device, window, surface);
    
    static constexpr uint32_t NUM_CHILDREN = 1;
    const char* sample_texture = "sample_texture.png";
    
    //Create the root node of the sample graph
    vk::graph<NUM_CHILDREN> sample_graph(&device, material_store, swapchain);
    
    //create one node: the display texture node
    glm::vec2 dims = {swapchain.get_vk_swap_extent().width, swapchain.get_vk_swap_extent().height };
    display_texture_2d<NUM_CHILDREN> texture_node(&device, &swapchain, (uint32_t)dims.x, (uint32_t)dims.y, sample_texture);
    
    //add node to the graph
    sample_graph.add_child(texture_node);
    
    //init all nodes in the graph
    sample_graph.init();
    
    
    //next_swap is a variable that stores which of of the swapchain images we are rendering.
    //Remember that in vulkan, the cpu doesn't have to wait for rendering to finish before moving forward, unlike opengl.
    //The graph we build uses this to its advantage, this is the reason why we need to tell it which of the swapchain images
    //we are rendering to.
    
    int next_swap = 0;
    while (!glfwWindowShouldClose(window) )
    {
        glfwPollEvents();
       
        //every frame, we are going to call the update function on every node, this is where the 
        //material arguments get updated, and these argument are then used at rendering time
        sample_graph.update(*app.perspective_camera, next_swap);
        
        //here we record vulkan rendering commands, or compute commands
        sample_graph.record(next_swap);
        
        //here we execute the command we just recorded.
        sample_graph.execute(next_swap);
        next_swap = ++next_swap % vk::NUM_SWAPCHAIN_IMAGES;
    }
    
    device.wait_for_all_operations_to_finish();
    sample_graph.destroy_all();
    
    vkDestroySurfaceKHR(device._instance, surface, nullptr);
    device.destroy();
    
    glfwDestroyWindow(window);
    glfwTerminate();
```

I will make sure to update [**main.mm**](https://github.com/phonowiz/vulkan-demos/blob/master/vulkan-demos/main.mm) with example graphs you can use to learn more about them.  


## Voxel Cone Tracing Example
Here are screenshots of my deferred renderings, these will be used for voxel cone tracing. 

#### Albedo
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/albedo.png">

#### Depth
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/depth.png">

#### Direct Lighting
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/direct-lighting.png">

#### Normals (Only 2 channels are needed to store normals)
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/normals.png">

#### 3D Texture Visualization
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/3d-texture visualization.png">

#### Ambient Occlusion
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/ambient_occlusion.png">

#### Variance Shadow Mapping
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/vsm.png">

#### Direct+Ambient Lighting with Ambient Occlusion
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-demos/screenshots/ambient+direct.png">

