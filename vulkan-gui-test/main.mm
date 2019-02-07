// VulkanTutorial.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#define VK_USE_PLATFORM_MACOS_MVK
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_SILENT_WARNINGS
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "vulkan_utils.h"
#include "easy_image.h"
#include "depth_image.h"
#include "vertex.h"
#include "mesh.h"
#include "create_swapchain.hpp"


#include <vector>
#include <array>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

///an excellent summary of vulkan can be found here:
//https://renderdoc.org/vulkan-in-30-minutes.html

VkInstance instance;
VkSurfaceKHR surface;
VkDevice device;
VkShaderModule shaderModuleVert;
VkShaderModule shaderModuleFrag;
VkPipelineLayout pipelineLayout;
VkRenderPass renderPass;
VkPipeline pipeline;
VkCommandPool commandPool;
VkCommandBuffer *commandBuffers;
VkSemaphore semaphoreImageAvailable;
VkSemaphore semaphoreRenderingDone;
VkPipelineShaderStageCreateInfo shaderStages[2];
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferDeviceMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferDeviceMemory;
VkBuffer uniformBuffer;
VkDeviceMemory uniformBufferMemory;
GLFWwindow *window;
VkPhysicalDevice physicalDevice;
SwapChainData swapChainData;

uint32_t width = 1024;
uint32_t height = 768;
//const VkFormat ourFormat = VK_FORMAT_B8G8R8A8_UNORM; //TODO civ

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 lightPosition;
};

UniformBufferObject ubo;

VkDescriptorSetLayout descriptorSEtLayout;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;


EasyImage shroomImage;
DepthImage depthImage;
Mesh dragonMesh;

//TWO PLANES, FOR TESTING PURPOSES
//std::vector<Vertex>& vertices;
//std::vector<Vertex> vertices = {
//    Vertex({-0.5f, -.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}),
//    Vertex({0.5f, .5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),
//    Vertex({-.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}),
//    Vertex({0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}),
//
//    Vertex({-0.5f, -.5f, -.50f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}),
//    Vertex({0.5f, .5f, -.50f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),
//    Vertex({-.5f, 0.5f, -.50f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}),
//    Vertex({0.5f, -0.5f, -.50f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f})
//};

//std::vector<uint32_t> indices = {
////        4,5,6,4,7,5,
//    0,1,2,0,3,1,
//        4,5,6,4,7,5
//};
//std::vector<uint32_t>& indices;

void printStats(VkPhysicalDevice &device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    
    std::cout << "Name:                     " << properties.deviceName << std::endl;
    uint32_t apiVer = properties.apiVersion;
    std::cout << "API Version:              " << VK_VERSION_MAJOR(apiVer) << "." << VK_VERSION_MINOR(apiVer) << "." << VK_VERSION_PATCH(apiVer) << std::endl;
    std::cout << "Driver Version:           " << properties.driverVersion << std::endl;
    std::cout << "Vendor ID:                " << properties.vendorID << std::endl;
    std::cout << "Device ID:                " << properties.deviceID << std::endl;
    std::cout << "Device Type:              " << properties.deviceType << std::endl;
    std::cout << "discreteQueuePriorities:  " << properties.limits.discreteQueuePriorities << std::endl;
    
    VkPhysicalDeviceFeatures features;
    
    vkGetPhysicalDeviceFeatures(device, &features);
    std::cout << "Geometry Shader:          " << features.geometryShader << std::endl;
    
    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(device, &memProp);
    
    uint32_t amountOfQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueueFamilies, nullptr);
    VkQueueFamilyProperties *familyProperties = new VkQueueFamilyProperties[amountOfQueueFamilies];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueueFamilies, familyProperties);
    
    std::cout << "Amount of Queue Families: " << amountOfQueueFamilies << std::endl;
    
    for (int i = 0; i < amountOfQueueFamilies; i++) {
        std::cout << std::endl;
        std::cout << "Queue Family #" << i << std::endl;
        std::cout << "VK_QUEUE_GRAPHICS_BIT       " << ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_COMPUTE_BIT        " << ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_TRANSFER_BIT       " << ((familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_SPARSE_BINDING_BIT " << ((familyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << std::endl;
        std::cout << "Queue Count: " << familyProperties[i].queueCount << std::endl;
        std::cout << "Timestamp Valid Bits: " << familyProperties[i].timestampValidBits << std::endl;
        uint32_t width = familyProperties[i].minImageTransferGranularity.width;
        uint32_t height = familyProperties[i].minImageTransferGranularity.height;
        uint32_t depth = familyProperties[i].minImageTransferGranularity.depth;
        std::cout << "Min Image Transfer Granularity: " << width << ", " << height << ", " << depth << std::endl;
    }
    
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilities);
    std::cout << "Surface capabilities: " << std::endl;
    std::cout << "\tminImageCount: " << surfaceCapabilities.minImageCount << std::endl;
    std::cout << "\tmaxImageCount: " << surfaceCapabilities.maxImageCount << std::endl;
    std::cout << "\tcurrentExtent: " << surfaceCapabilities.currentExtent.width << "/" << surfaceCapabilities.currentExtent.height << std::endl;
    std::cout << "\tminImageExtent: " << surfaceCapabilities.minImageExtent.width << "/" << surfaceCapabilities.minImageExtent.height << std::endl;
    std::cout << "\tmaxImageExtent: " << surfaceCapabilities.maxImageExtent.width << "/" << surfaceCapabilities.maxImageExtent.height << std::endl;
    std::cout << "\tmaxImageArrayLayers: " << surfaceCapabilities.maxImageArrayLayers << std::endl;
    std::cout << "\tsupportedTransforms: " << surfaceCapabilities.supportedTransforms << std::endl;
    std::cout << "\tcurrentTransform: " << surfaceCapabilities.currentTransform << std::endl;
    std::cout << "\tsupportedCompositeAlpha: " << surfaceCapabilities.supportedCompositeAlpha << std::endl;
    std::cout << "\tsupportedUsageFlags: " << surfaceCapabilities.supportedUsageFlags << std::endl;
    
    uint32_t amountOfFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &amountOfFormats, nullptr);
    VkSurfaceFormatKHR *surfaceFormats = new VkSurfaceFormatKHR[amountOfFormats];
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &amountOfFormats, surfaceFormats);
    
    std::cout << std::endl;
    std::cout << "Amount of Formats: " << amountOfFormats << std::endl;
    for (int i = 0; i < amountOfFormats; i++) {
        std::cout << "Format: " << surfaceFormats[i].format << std::endl;
        std::cout << "Color Space: " << surfaceFormats[i].colorSpace << std::endl;
    }
    
    uint32_t amountOfPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &amountOfPresentationModes, nullptr);
    VkPresentModeKHR *presentModes = new VkPresentModeKHR[amountOfPresentationModes];
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &amountOfPresentationModes, presentModes);
    
    std::cout << std::endl;
    std::cout << "Amount of Presentation Modes: " << amountOfPresentationModes << std::endl;
    for (int i = 0; i < amountOfPresentationModes; i++) {
        std::cout << "Supported presentation mode: " << presentModes[i] << std::endl;
    }
    
    
    std::cout << std::endl;
    delete[] familyProperties;
    delete[] surfaceFormats;
    delete[] presentModes;
}


std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    
    if (file) {
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> fileBuffer(fileSize);
        file.seekg(0);
        file.read(fileBuffer.data(), fileSize);
        file.close();
        return fileBuffer;
    }
    else {
        throw std::runtime_error("Failed to open file!!!");
    }
}


void recreateSwapchain();

void onWindowResized(GLFWwindow * window, int w, int h)
{
    if( w != 0 && h != 0)
    {
        
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
        
        w = std::min(w, static_cast<int>(surfaceCapabilities.maxImageExtent.width));
        h = std::min(h, static_cast<int>(surfaceCapabilities.maxImageExtent.height));
        
        w = std::max(w, static_cast<int>(surfaceCapabilities.minImageExtent.width));
        h = std::max(h, static_cast<int>(surfaceCapabilities.minImageExtent.height));
        
        width = w;
        height = h;
        recreateSwapchain();
    }
}


void startGlfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    
    window = glfwCreateWindow(width, height, "Vulkan Tutorial", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onWindowResized);
}

void createShaderModule(const std::vector<char>& code, VkShaderModule *shaderModule) {
    VkShaderModuleCreateInfo shaderCreateInfo;
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.pNext = nullptr;
    shaderCreateInfo.flags = 0;
    shaderCreateInfo.codeSize = code.size();
    shaderCreateInfo.pCode = (uint32_t*)code.data();
    
    VkResult result = vkCreateShaderModule(device, &shaderCreateInfo, nullptr, shaderModule);
    ASSERT_VULKAN(result);
}


void createInstance()
{
    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "Vulkan Tutorial";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    appInfo.pEngineName = "Super Vulkan Engine Turbo Mega";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    const std::vector<const char*> validationLayers = {
#ifndef __APPLE__
        "VK_LAYER_LUNARG_standard_validation"
#endif
    };
    
    uint32_t amountOfGlfwExtensions = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&amountOfGlfwExtensions);
    
    
    
    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = (uint32_t)validationLayers.size();
    instanceInfo.ppEnabledLayerNames = validationLayers.data();
    instanceInfo.enabledExtensionCount = amountOfGlfwExtensions;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;
    
    
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    ASSERT_VULKAN(result);
    
}

void printInstanceLayers()
{
    uint32_t amountOfLayers = 0;
    vkEnumerateInstanceLayerProperties(&amountOfLayers, nullptr);
    VkLayerProperties *layers = new VkLayerProperties[amountOfLayers];
    vkEnumerateInstanceLayerProperties(&amountOfLayers, layers);
    
    std::cout << "Amount of Instance Layers: " << amountOfLayers << std::endl;
    for (int i = 0; i < amountOfLayers; i++) {
        std::cout << std::endl;
        std::cout << "Name:         " << layers[i].layerName << std::endl;
        std::cout << "Spec Version: " << layers[i].specVersion << std::endl;
        std::cout << "Impl Version: " << layers[i].implementationVersion << std::endl;
        std::cout << "Description:  " << layers[i].description << std::endl;
    }
    delete[] layers;
}

void printInstanceExtensions()
{
    uint32_t amountOfExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, nullptr);
    VkExtensionProperties *extensions = new VkExtensionProperties[amountOfExtensions];
    vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, extensions);
    
    std::cout << std::endl;
    std::cout << "Amount of Extensions: " << amountOfExtensions << std::endl;
    for (int i = 0; i < amountOfExtensions; i++) {
        std::cout << std::endl;
        std::cout << "Name: " << extensions[i].extensionName << std::endl;
        std::cout << "Spec Version: " << extensions[i].specVersion << std::endl;
    }
    
    delete[] extensions;
}



void createGLfwWindowSurface()
{
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    ASSERT_VULKAN(result);
}

void printStatsOfAllPhysicalDevices(std::vector<VkPhysicalDevice>& physicalDevices)
{
    for (int i = 0; i < physicalDevices.size(); i++) {
        printStats(physicalDevices[i]);
        std::cout << "-----------------------------" << std::endl;
    }
}

void createImageViews(VkDevice device, SwapChainData &swapChainData)
{
    swapChainData.swapChainImageViews.resize(swapChainData.swapChainImages.size());
    
    for (size_t i = 0; i < swapChainData.swapChainImages.size(); i++)
    {
        createImageView(device, swapChainData.swapChainImages[i], swapChainData.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, swapChainData.swapChainImageViews[i]);
    }
}

void createRenderPass(SwapChainData& swapChainData)
{
    
    VkAttachmentDescription attachmentDescription;
    attachmentDescription.flags = 0;
    attachmentDescription.format = swapChainData.swapChainImageFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference attachmentReference;
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription depthAttachment = DepthImage::getDepthAttachment(physicalDevice);
    
    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    
    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &attachmentReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    
    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dependencyFlags = 0;
    
    std::vector<VkAttachmentDescription> attachments;
    attachments.push_back(attachmentDescription);
    attachments.push_back(depthAttachment);
    
    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;
    
    VkResult result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
    ASSERT_VULKAN(result);
}

void createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    
    //this is the uniform buffer containing model/view/projection information
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
    
    //this is bound to binding=1 at is the texture we sample
    VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding = {};
    samplerDescriptorSetLayoutBinding.binding = 1;
    samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerDescriptorSetLayoutBinding.descriptorCount = 1;
    samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;
    
    std::vector<VkDescriptorSetLayoutBinding> descriptorSets;
    descriptorSets.push_back(descriptorSetLayoutBinding);
    descriptorSets.push_back(samplerDescriptorSetLayoutBinding);
    
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = descriptorSets.size();
    descriptorSetLayoutCreateInfo.pBindings = descriptorSets.data();
    
    VkResult result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSEtLayout);
    ASSERT_VULKAN(result);
}


void createPipeline()
{
    
    /////BUILD SHADERS
    std::string baseDir;
    getBaseDir( baseDir );
    std::string vertShader = baseDir + "shaders/triangle.vert";
    std::string fileContents;
    readFile(fileContents, vertShader);
    
    init_shaders(device, shaderStages[0], VK_SHADER_STAGE_VERTEX_BIT, fileContents.c_str());
    
    fileContents.clear();
    std::string fragShader = baseDir + "shaders/triangle.frag";
    readFile(fileContents, fragShader);
    init_shaders(device, shaderStages[1], VK_SHADER_STAGE_FRAGMENT_BIT, fileContents.c_str());
    
    
    auto vertexBindingDescription = Vertex::getBindingDescription();
    auto vertexAttributeDescription = Vertex::getAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.pNext = nullptr;
    vertexInputCreateInfo.flags = 0;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescription.size();
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo;
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.pNext = nullptr;
    inputAssemblyCreateInfo.flags = 0;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
    
    glfwGetFramebufferSize(window, (int*)&width, (int*)&height);
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = { width, height };
    
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.pNext = nullptr;
    viewportStateCreateInfo.flags = 0;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo;
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.pNext = nullptr;
    rasterizationCreateInfo.flags = 0;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationCreateInfo.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo;
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.pNext = nullptr;
    multisampleCreateInfo.flags = 0;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.pSampleMask = nullptr;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;
    
    VkPipelineDepthStencilStateCreateInfo dephtStencilStateCreateInfo = DepthImage::getDepthStencilStateCreateInfoOpaque();
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.pNext = nullptr;
    colorBlendCreateInfo.flags = 0;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;
    
    VkPushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(VkBool32);
    
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSEtLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    
    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
    ASSERT_VULKAN(result);

    
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pNext = nullptr;
    dynamicStateCreateInfo.flags = 0;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;
    
    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pTessellationState = nullptr;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &dephtStencilStateCreateInfo;;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;
    
    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
    ASSERT_VULKAN(result);
}

void createFrameBuffers(SwapChainData &swapChainData)
{
    glfwGetFramebufferSize(window, (int*)&width, (int*)&height);

    swapChainData.swapChainFramebuffers.resize(swapChainData.swapChainImages.size());


    
    for (size_t i = 0; i < swapChainData.swapChainFramebuffers.size(); i++)
    {
        VkImageView depthImageView = depthImage.getImageView();
        std::array<VkImageView, 2> attachmentViews = {swapChainData.swapChainImageViews[i], depthImageView};
        
        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        framebufferCreateInfo.pAttachments = attachmentViews.data();
        framebufferCreateInfo.width = width;
        framebufferCreateInfo.height = height;
        framebufferCreateInfo.layers = 1;
        
        VkResult result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapChainData.swapChainFramebuffers[i]);
        ASSERT_VULKAN(result);
    }
}

void createCommandPool()
{
    
    VkCommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = 0;
    commandPoolCreateInfo.queueFamilyIndex = 0; //TODO: FIND OUT WHAT QUEUE WE PUT HERE;
    
    VkResult result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
    ASSERT_VULKAN(result);
}

void createDepthImage()
{
    depthImage.create(device, physicalDevice, commandPool, graphicsQueue, width, height);
}
void createCommandBuffers(SwapChainData& swapChainData)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(swapChainData.swapChainImages.size());
    
    commandBuffers = new VkCommandBuffer[swapChainData.swapChainImages.size()];
    VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers);
    ASSERT_VULKAN(result);
}

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dest, VkDeviceSize size)
{

    VkCommandBuffer commandBuffer = startSingleTimeCommandBuffer(device, commandPool);

    VkBufferCopy bufferCopy = {};
    bufferCopy.dstOffset = 0;
    bufferCopy.srcOffset = 0;
    bufferCopy.srcOffset = 0;
    bufferCopy.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dest, 1, &bufferCopy);

    endSingleTimeCommandBuffer(device, queue, commandPool, commandBuffer);

}

void loadTexture()
{
    std::string baseDir;
    getBaseDir( baseDir );
    std::string texture = baseDir + "textures/mario.png";
    shroomImage.load(texture.c_str());
    shroomImage.upload(device, physicalDevice, commandPool, graphicsQueue);
    
}

void loadMesh()
{
    std::string baseDir;
    getBaseDir( baseDir );
    std::string model = baseDir + "models/dragon.obj";
    dragonMesh.create(model.c_str());
}

void createVertexBuffer()
{
    createAndUploadBuffer(device, physicalDevice, graphicsQueue, commandPool,
                          dragonMesh.getVertices(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferDeviceMemory);
}

void createIndexBuffer()
{
    createAndUploadBuffer(device, physicalDevice, graphicsQueue, commandPool,
                          dragonMesh.getIndices(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferDeviceMemory);
}

void createUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(ubo);
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniformBuffer,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBufferMemory);
}

void createDescriptorPool()
{
    VkDescriptorPoolSize descriptorPoolSize ={};
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize.descriptorCount = 1;
    
    VkDescriptorPoolSize samplerPoolSize;
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = 1;
    
    std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
    descriptorPoolSizes.push_back(descriptorPoolSize);
    descriptorPoolSizes.push_back(samplerPoolSize);
    
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};

    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = 1;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
    
    ASSERT_VULKAN(result);
}

void createDescriptorSet()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSEtLayout;
    
    VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet);
    ASSERT_VULKAN(result);
    
    VkDescriptorBufferInfo descriptorbufferInfo = {};
    descriptorbufferInfo.buffer = uniformBuffer;
    descriptorbufferInfo.offset = 0;
    descriptorbufferInfo.range = sizeof(ubo);
    
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext = nullptr;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pBufferInfo = &descriptorbufferInfo;
    descriptorWrite.pTexelBufferView = nullptr;
    
    VkDescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.sampler = shroomImage.getSampler();
    descriptorImageInfo.imageView = shroomImage.getImageView();
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    VkWriteDescriptorSet descriptorSampler;
    descriptorSampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSampler.pNext = nullptr;
    descriptorSampler.dstSet = descriptorSet;
    descriptorSampler.dstBinding = 1;
    descriptorSampler.dstArrayElement = 0;
    descriptorSampler.descriptorCount = 1;
    descriptorSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSampler.pImageInfo = &descriptorImageInfo;
    descriptorSampler.pBufferInfo = nullptr;
    descriptorSampler.pTexelBufferView = nullptr;
    
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    writeDescriptorSets.push_back(descriptorWrite);
    writeDescriptorSets.push_back(descriptorSampler);
    vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

void recordCommandBuffers()
{
    
    
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    
    
    
    for (size_t i = 0; i < swapChainData.swapChainImages.size(); i++)
    {
        VkResult result = vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);
        ASSERT_VULKAN(result);
        
        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = swapChainData.swapChainFramebuffers[i];//framebuffers[i];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = { width, height };
        VkClearValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        VkClearValue depthClearValue = {1.0f, 0.0f};
        
        std::array<VkClearValue,2> clearValues;
        clearValues[0] = clearValue;
        clearValues[1] = depthClearValue;
        
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();
        
        
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkBool32 usePhong = VK_FALSE;
        vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(usePhong), &usePhong);
        
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);
        
        VkRect2D scissor;
        scissor.offset = { 0, 0};
        scissor.extent = { width, height};
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);
        
        VkDeviceSize offsets[] = { 0 };
        //vkCmdBindVertexBuffers(commandBuffers[i], 0, &vertexBuffer, offsets);
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        
        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(dragonMesh.getIndices().size()), 1, 0, 0, 0);
        
        vkCmdEndRenderPass(commandBuffers[i]);
        
        result = vkEndCommandBuffer(commandBuffers[i]);
        ASSERT_VULKAN(result);
        
        //clearValues.clear();
    }
}

void createSemaphores()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    
    VkResult result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphoreImageAvailable);
    ASSERT_VULKAN(result);
    result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphoreRenderingDone);
    ASSERT_VULKAN(result);
}

void startVulkan() {
    
    createInstance();
    printInstanceLayers();
    printInstanceExtensions();
    createSurface(instance, window, surface);
    physicalDevice = pickPhysicalDevice(instance, surface);
    
    createLogicalDevice(physicalDevice, device, surface);
    createSwapChain(physicalDevice, device, surface, *window, swapChainData);
    createImageViews(device, swapChainData);
    createRenderPass(swapChainData);
    createDescriptorSetLayout();
    createPipeline();
    createCommandPool();
    createDepthImage();
    createFrameBuffers(swapChainData);
    createCommandBuffers(swapChainData);
    loadTexture();
    loadMesh();
    createVertexBuffer();
    createIndexBuffer();
    
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    
    recordCommandBuffers();
    createSemaphores();
}

void recreateSwapchain()
{
    vkDeviceWaitIdle(device);
    
//    vkDestroySemaphore(device, semaphoreImageAvailable, nullptr);
//    vkDestroySemaphore(device, semaphoreRenderingDone, nullptr);
    
    depthImage.destroy();
    vkFreeCommandBuffers(device, commandPool, swapChainData.swapChainImages.size(), commandBuffers);
    
    for (size_t i = 0; i < swapChainData.swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapChainData.swapChainFramebuffers[i], nullptr);
    }
    
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (int i = 0; i < swapChainData.swapChainImageViews.size(); i++)
    {
        vkDestroyImageView(device, swapChainData.swapChainImageViews[i], nullptr);
    }
    
    VkSwapchainKHR oldSwapchain = swapChainData.swapChain;

    createSwapChain(physicalDevice, device, surface, *window, swapChainData);
    createImageViews(device, swapChainData);
    createRenderPass(swapChainData);
    createDepthImage();
    createFrameBuffers(swapChainData);
    createCommandPool();
    createCommandBuffers(swapChainData);
    recordCommandBuffers();
    createSemaphores();
    
    vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
}

void drawFrame(SwapChainData& swapChainData) {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChainData.swapChain, std::numeric_limits<uint64_t>::max(), semaphoreImageAvailable, VK_NULL_HANDLE, &imageIndex);
    
    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphoreImageAvailable;
    VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(commandBuffers[imageIndex]);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphoreRenderingDone;
    
    VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_VULKAN(result);
    
    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphoreRenderingDone;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChainData.swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    vkDeviceWaitIdle(device);
    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
    ASSERT_VULKAN(result);
}

std::chrono::time_point gameStartTime = std::chrono::high_resolution_clock::now();
void updateMVP()
{
    std::chrono::time_point frameTime = std::chrono::high_resolution_clock::now();
    float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>( frameTime - gameStartTime ).count()/1000.0f;
    
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), timeSinceStart * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), width/(float)height, 0.01f, 10.0f);
    
    /*
     GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
     The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
     If you don't do this, then the image will be rendered upside down.
     */
    projection[1][1] *= -1.0f;

    glm::vec4 temp =(glm::rotate(glm::mat4(1.0f), timeSinceStart * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 3.0f, 1.0f, 0.0f));
    ubo.lightPosition.x = temp.x;
    ubo.lightPosition.y = temp.y;
    ubo.lightPosition.z = temp.z;
    ubo.model = model;
    ubo.view = view;
    ubo.projection = projection;
    
    void* data = nullptr;
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBufferMemory);
    
    
}

void gameLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        updateMVP();
        drawFrame(swapChainData);
    }
}

void shutdownVulkan() {
    vkDeviceWaitIdle(device);
    
    depthImage.destroy();
    vkDestroyDescriptorSetLayout(device, descriptorSEtLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    
    vkFreeMemory(device, uniformBufferMemory, nullptr);
    vkDestroyBuffer(device, uniformBuffer, nullptr);
    
    vkFreeMemory(device, indexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    
    
    vkFreeMemory(device, vertexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    
    shroomImage.destroy();
    
    vkDestroySemaphore(device, semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(device, semaphoreRenderingDone, nullptr);
    
    vkFreeCommandBuffers(device, commandPool, swapChainData.swapChainImages.size(), commandBuffers);
    delete[] commandBuffers;
    
    vkDestroyCommandPool(device, commandPool, nullptr);
    
    for (size_t i = 0; i < swapChainData.swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapChainData.swapChainFramebuffers[i], nullptr);
    }
    
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (int i = 0; i < swapChainData.swapChainImageViews.size(); i++) {
        vkDestroyImageView(device, swapChainData.swapChainImageViews[i], nullptr);
    }
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
    vkDestroySwapchainKHR(device, swapChainData.swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void shutdownGlfw() {
    glfwDestroyWindow(window);
    
    glfwTerminate();
}


int main()
{
    startGlfw();
    startVulkan();
    gameLoop();
    shutdownVulkan();
    shutdownGlfw();
    
    return 0;
}





