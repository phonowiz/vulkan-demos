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

Here is an example of how you would update shader parameters every frame:

```c++
    std::chrono::time_point frameTime = std::chrono::high_resolution_clock::now();
    float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>( frameTime - gameStartTime ).count()/1000.0f;
    
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), timeSinceStart * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), width/(float)height, 0.01f, 10.0f);
    
    projection[1][1] *= -1.0f;
    
    glm::vec4 temp =(glm::rotate(glm::mat4(1.0f), timeSinceStart * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 3.0f, 1.0f, 0.0f));

    vk::shader_parameter::shader_params_group& vertexParams =   standardMat->get_uniform_parameters(vk::material::parameter_stage::VERTEX, 0);
    
    vertexParams["model"] = model;
    vertexParams["view"] = view;
    vertexParams["projection"] = projection;
    vertexParams["lightPosition"] = temp;


    standardMat->set_image_sampler(texture, "tex", vk::material::parameter_stage::FRAGMENT, 1);
    
    
    standardMat->commit_parameters_to_gpu();

```

Here is a screenshot of my first Vulkan scene:

<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/dragon.png">

At 1024x768 resolution and about 800,000 polygons, this sample app can run at around 150 frames a second, more than twice as fast as what OpenGL can do on my MacBook Pro 2015 with the same scene.  That is amazing! 

