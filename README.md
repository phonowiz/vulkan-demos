# vulkan-gui-test

This is my test bed for all things vulkan.  It is project using MoltenVK on a mac and a setup using Xcode.  Vulkan projects are not very common on the mac and I would like to set up an example for others to learn from.

Purpose
----------
I want to create renderer that I could use to prototype ideas quickly, game engines are very good for this, but I'd rather implement everything from scratch because you learn more that way.  The need came about because Apple will deprecate OpenGL soon, and I need to port my other project to Vulkan, you can check it out here: https://github.com/phonowiz/voxel-cone-tracing

This code is not very organized as of now, there is still lots of code moving all over the place as I try what I think is right.  The meaningful work can be found in the folder "vulkan_wrapper" which you can look at here: https://github.com/phonowiz/vulkan-gui-test/tree/master/vulkan-gui-test/vulkan_wrapper.  For an example of how I think the renderer api will work, check this out:

```c++
vk::PhysicalDevice device;

createSurface(device._instance, window, surface);

device.createLogicalDevice(surface);

vk::SwapChain swapChain(&device, window);
vk::MaterialStore materialStore;

materialStore.createStore(&device);

standardMat = materialStore.GET_MAT<vk::Material>("standard");

vk::Mesh mesh( "dragon.obj", &device );

vk::plane plane(&device);
plane.create();

vk::Texture2D mario(&device, "mario.png");

vk::Renderer renderer(&device,window, &swapChain, standardMat);

renderer.addMesh(&mesh);
renderer.init();

gameLoop(renderer);

device.waitForllOperationsToFinish();
swapChain.destroy();
materialStore.destroy();
mesh.destroy();
plane.destroy();
renderer.destroy();
device.destroy();
```

The naming style is still not consistent, but you get the idea.  Here is a screenshot of my first Vulkan scene:

<img src="https://github.com/phonowiz/vulkan-gui-test/blob/master/vulkan-gui-test/screenshots/dragon.png">

At 1024x768 resolution and about 800,000 polygons, this sample app can run at around 150 frames a second, more than twice as fast as what OpenGL can do on my MacBook Pro 2015 with the same scene.  That is amazing! 

