# Vulkan Renderer

This is my test bed for all things vulkan.  It is project using MoltenVK on a mac and a setup using Xcode.  Vulkan projects are not very common on the mac and I would like to set up an example for others to learn from.

Purpose
----------
I want to create renderer that I could use to prototype ideas quickly, game engines are very good for this, but I'd rather implement everything from scratch because you learn more that way.  The need came about because Apple will deprecate OpenGL soon, and I need to port my other project to Vulkan, you can check it out here: https://github.com/phonowiz/voxel-cone-tracing

This code is not very organized as of now, there is still lots of code moving all over the place as I try what I think is right.  The meaningful work can be found in the folder "vulkan_wrapper" which you can look at here: https://github.com/phonowiz/vulkan-gui-test/tree/master/vulkan-gui-test/vulkan_wrapper.  Inside of the classes you'll notice that members are all public, that's because am still settling down on what is public vs private.   


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
    swapChain.destroy();
    materialStore.destroy();
    mesh.destroy();
    plane.destroy();
    renderer.destroy();
    device.destroy();
```

Here is a screenshot of my first Vulkan scene:

<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/dragon.png">

At 1024x768 resolution and about 800,000 polygons, this sample app can run at around 150 frames a second, more than twice as fast as what OpenGL can do on my MacBook Pro 2015 with the same scene.  That is amazing! 

