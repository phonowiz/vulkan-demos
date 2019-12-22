# Mac OS Vulkan Renderer

This is my test bed for all things vulkan.  It is project using MoltenVK on a mac and a setup using Xcode.  Vulkan projects are not very common on the mac and I would like to set up an example for others to learn from.

Purpose
----------
I want to create renderer that I could use to prototype ideas quickly, game engines are very good for this, but I'd rather implement everything from scratch because you learn more that way.  The need came about because Apple will deprecate OpenGL soon, and I need to port my other project to Vulkan, you can check it out here: https://github.com/phonowiz/voxel-cone-tracing

This code is not very organized as of now, there is still lots of code moving all over the place as I try what I think is right.  The meaningful work can be found in the folder "vulkan_wrapper" which you can look at here: https://github.com/phonowiz/vulkan-gui-test/tree/master/vulkan-gui-test/vulkan_wrapper.  


For an example of how I think the renderer api will work, check this out:



```c++
    vk::device device;
    
    VkResult res = glfwCreateWindowSurface(device._instance, window, nullptr, &surface);
    assert(res == VK_SUCCESS);
    
    device.create_logical_device(surface);
    
    vk::swapchain swapchain(&device, window, surface);
    vk::material_store material_store;
    
    material_store.create(&device);
    
    standard_mat = material_store.GET_MAT<vk::material>("standard");

    vk::mesh mesh( "dragon.obj", &device );
    
    vk::renderer renderer(&device,window, &swapchain, standard_mat);

    renderer.add_mesh(&mesh);
    renderer.init();
    
    gameLoop(renderer);
    
    device.wait_for_all_operations_to_finish();
    swapchain.destroy();
    material_store.destroy();
    mesh.destroy();
    plane.destroy();
    renderer.destroy();
    device.destroy();
```

Here is an example of how you would update shader parameters every frame:

```c++

    glm::mat4 projection = //code that updates projection matrix...
    
    
    glm::vec4 light_pos = //code that updates light position

    int binding = 0;
    vk::shader_parameter::shader_params_group& vertex_params =   
        standard_mat->get_uniform_parameters(vk::material::parameter_stage::VERTEX, binding);
    
    //the parameters need to be listed in the same order they are listed in the shader struct that declares them.
    vertex_params["model"] = model;
    vertex_params["view"] = view;
    vertex_params["projection"] = projection;
    vertex_params["lightPosition"] = light_pos;

    
    //texture variable is of type vk::texture_2d
    binding = 1;
    standard_mat->set_image_sampler(texture, "tex", vk::material::parameter_stage::FRAGMENT, binding);
    standard_mat->commit_parameters_to_gpu();

```
If you have ever used vulkan api directly to do all of this, you know there is a ton of of stuff that goes on to make this snippet happen, I've managed to boil it down to less than 10 lines.

My philosophy is to make something that satisfies my needs specifically and only expose exactly what I need from Vulkan, the rest can stay hidden with default values. As my development gets more sophisticated, I'll keep exposing more and more of the API, just enough to get what I need done.  It keeps things simpler. 

## Deferred Rendering
Here are screenshots of my deferred renderings, these will be used for voxel cone tracing. 

#### Albedo
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/albedo.png">

#### Depth
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/depth.png">

#### Direct Lighting
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/direct-lighting.png">

#### Normals
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/normals.png">

#### 3D Texture Visualization
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/3d-texture visualization.png">
The streaks you see here are as result of the ray marching am doing to render the 3D texture.  This view is mostly for debugging purposes and designed to show me what the 3D texture content is, how I visualize this doesn't contribute to the final cone traced image, but the contents of the 3D texture will contrinbute to final cone traced image.   

#### Ambient Occlusion
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/ambient_occlusion.png">

#### Direct Lighting with Ambient Occlusion
<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/ambient+direct.png">

