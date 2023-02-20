/// This is an introduction to the Vulkan core API.
/// The introduction targets an audience with background in OpenGL without previous knowledge of Vulkan.
/// For a reader belonging to this audience, Vulkan can be a bit intimidating at first glance.
/// The offical specs (https://registry.khronos.org/vulkan/specs/1.3-khr-extensions/html/) and the official tutorial (https://vulkan-tutorial.com/) are very dense.
/// In order to learn Vulkan you will need to find learning resources on the right level.
/// Below is a list of good reading material (should be read in that order).
///
///     1. Read up on how to learn Vulkan: https://www.jeremyong.com/c++/vulkan/graphics/rendering/2018/03/26/how-to-learn-vulkan/
///     2. Step through the Vulkan Guide: https://github.com/KhronosGroup/Vulkan-Guide
///     3. Read through the Intel tutorial (skip the dynamic library stuff): https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-1.html
///
/// We will walk through how to setup a minimal graphics pipeline and render an cube to image on disk.
/// As we proceed, various core Vulkan concepts will be introduced and their rationale explained.
///
/// We will use pure C as our language of choice because Vulkan is a C API.
/// In my opinion, mixing in a different language (usually C++) makes it harder to learn Vulkan. It is easier to learn it with a C mindset.
///
/// I highly recommend to tag along and write the code yourself, no copy pasting.
/// Make sure you have a good setup with and editor and autocompletion, or your fingers will hate you.
/// I personally use Vim with CoC plugin for autocompletion using Clangd 15.
/// Spin up a web browser with some documentation
///
///     https://devdocs.io/vulkan
///
/// Make sure to install the necessary tools. On Ubuntu you should install the `vulkan-tools` package which gives you the `vulkaninfo` command.
///
/// Notes about the Vulkan API
///
///     1. The API makes almost no assumption about its usage.
///        This is the reason why so much code is required for so little action.
///     2. The API is decoupled. Almost every entity can be created with minimal information about other entities.
///        For example, a framebuffer can be created independently of a render pass.
///        A render pass describes dependencies between attachments in render sub passes, and a framebuffer describes which images should be used as attachments.
///        Hence, different framebuffers can be used with a single render pass, and different render passes can be used with a single framebuffer.

#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>

#ifndef BUILD_TYPE
#define BUILD_TYPE ""
#endif

#define VERTEX_SHADER_SOURCE_PATH "out/" BUILD_TYPE "/shader.vert.spv"
#define IMAGE_WIDTH 400
#define IMAGE_HEIGHT 400


#define STR(name) #name
#define CASE_STR(name) case name : return STR(name)

const char*
resultString(VkResult code)
{
    switch (code)
    {
        CASE_STR(VK_SUCCESS);
        CASE_STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        CASE_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        default: return "UNKNOWN";
    }
}


int main()
{
    /// First step is to create an instance object.
    /// This is where we specify global stuff such as info about our application, which validation layers and extensions that we want to load.
    /// The instance object is an opaque handle, which will be used to get physical devices with later on.
    ///
    /// We create the `VkInstance` by passing a `VkInstanceCreateInfo` to `vkCreateInstance`.
    /// Note that there is a corresponding `vkDestroyInstance` at the end of the program.
    /// This pattern is fundamental in Vulkan, the lifetime of all opaque objects follows this
    ///
    ///     1. Construct a `Vk...CreateInfo` object, where `...` is a placeholder for the type we are going to create
    ///     2. Call `vkCreate...` to create the object.
    ///     3. Call `vkDestroy...` when the object is not needed anymore.
    ///
    /// where the destruction usually happen in reverse order of creation, just like in OpenGL.
    ///
    /// For this program, we only specify the application info, which is minimal.
    /// The application info is used for things like telling Vulkan what API version we expect, and GPU vendors about our application.
    /// The latter usage can be used for application specific optimizations by a vendor, say for a game.
    ///
    /// Note that we have to explicitly set the type of the application info structure.
    /// That seems like a common point of error, and setting this wrong leads to undefined behaviour.
    /// The reason why the type exist is so that the drivers can dynamically figure out types from objects passed in.
#ifndef NDEBUG
    const uint32_t validationLayerCount = 1;
    const char * validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
#else
    const uint32_t validationLayerCount = 0;
    const char** validationLayers = NULL;
#endif
    printf("Creating instance with %d validation layers\n", validationLayerCount);
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_0
    };
    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = validationLayerCount,
        .ppEnabledLayerNames = validationLayers
    };
    VkInstance instance;
    if (vkCreateInstance(&instanceCreateInfo, NULL, &instance) != VK_SUCCESS)
    {
        printf("Failed to create instance\n");
        return EXIT_FAILURE;
    }


    /// After setting up the instance we are ready to define the device we will operate on.
    /// In Vulkan you can handle several physical devices, and we want to pick one of them.
    /// I am writing this on a laptop with 2 physical devices:
    ///
    ///   - The CPU with a software implementation of Vulkan called Lavapipe,
    ///   - The integrated graphics card
    ///
    /// which I can see by running `vulkaninfo | grep -A 7 VkPhysicalDeviceProperties`, which gives me
    ///
    ///     VkPhysicalDeviceProperties:
    ///     ---------------------------
    ///     	apiVersion     = 4202678 (1.2.182)
    ///     	driverVersion  = 88088582 (0x5402006)
    ///     	vendorID       = 0x8086
    ///     	deviceID       = 0x3ea0
    ///     	deviceType     = PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
    ///     	deviceName     = Intel(R) UHD Graphics 620 (WHL GT2)
    ///     --
    ///     VkPhysicalDeviceProperties:
    ///     ---------------------------
    ///     	apiVersion     = 4198582 (1.1.182)
    ///     	driverVersion  = 1 (0x0001)
    ///     	vendorID       = 0x10005
    ///     	deviceID       = 0x0000
    ///     	deviceType     = PHYSICAL_DEVICE_TYPE_CPU
    ///     	deviceName     = llvmpipe (LLVM 12.0.0, 256 bits)
    ///
    /// We want to select the graphics card as the physical device, and not the CPU.
    /// Communication with the physical device is done through commands sent over queues.
    /// A physical device can support a whole family of queues, each family with certain properties, such as support for graphical, compute and transfer commands.
    /// For each supported queue family, there can also be several queues.
    /// We will select the first queue family that supports both graphics and transfer commands, and we will only require one queue in that family.
    ///
    /// To select the appropriate physical device we will do the following
    ///
    ///     1. Enumerate all physical devices
    ///     2. Query each physical device for properties, check the device type and select the first suitable match.
    printf("Enumerating physical devices\n");
    uint32_t physicalDeviceCount;
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL) != VK_SUCCESS)
    {
        printf("Failed to enumerate physical devices\n");
        return EXIT_FAILURE;
    }
    VkPhysicalDevice physicalDevices[physicalDeviceCount];
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices) != VK_SUCCESS)
    {
        printf("Failed to enumerate physical devices\n");
        return EXIT_FAILURE;
    }
    printf("%d physical devices available\n", physicalDeviceCount);
    if (physicalDeviceCount == 0)
    {
        printf("Found no physical device\n");
        return EXIT_FAILURE;
    }

    printf("Selecting a suitable physical device\n");
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    uint32_t physicalDeviceIndex = 0;
    uint32_t queueFamilyIndex = 0;
    for (physicalDeviceIndex = 0; physicalDeviceIndex < physicalDeviceCount; ++physicalDeviceIndex)
    {
        physicalDevice = physicalDevices[physicalDeviceIndex];
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        if (!(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
              physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
        {
            printf("Physical device %d is not a GPU\n", physicalDeviceIndex);
            continue;
        }

        uint32_t queueFamiliesCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, NULL);
        VkQueueFamilyProperties queueFamilyProperties[queueFamiliesCount];
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamilyProperties);
        for (queueFamilyIndex = 0; queueFamilyIndex < queueFamiliesCount ; ++queueFamilyIndex) {
            VkQueueFlags flags = queueFamilyProperties[queueFamilyIndex].queueFlags;
            if ((flags & VK_QUEUE_GRAPHICS_BIT) && (flags & VK_QUEUE_TRANSFER_BIT)) {
                break;
            }
        }
        if (queueFamilyIndex == queueFamiliesCount)
        {
            printf("Found no suitable queue family for physical device %d\n", physicalDeviceIndex);
            continue;
        }

        break;
    }
    if (physicalDeviceIndex == physicalDeviceCount)
    {
        printf("Failed to find a suitable physical device\n");
        return EXIT_FAILURE;
    }
    printf("Selected physical device %d (%s)\n", physicalDeviceIndex, physicalDeviceProperties.deviceName);


    /// When we have found a suitable physical device we are ready to create a (logical) device from it.
    /// The logical device is an abstraction of a physical device with specified queues.
    /// The logical device owns all the queues it creates, and we can get a queue from it after creating the device.
    printf("Creating device\n");
    float queuePriority = 1;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
    };
    VkDevice device;
    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device) != VK_SUCCESS)
    {
        printf("Failed to create logical device\n");
        return EXIT_FAILURE;
    }
    VkQueue queue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);


    /// Setting up the graphics pipeline

    /// In order to render something, we need to define a render pass, a framebuffer and a graphics pipeline.
    printf("Creating render pass\n");
    VkAttachmentDescription attachmentDescription = {
        .flags = 0,
        .format = VK_FORMAT_D24_UNORM_S8_UINT,
        .samples = 1,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    VkAttachmentReference attachmentReference = {
        0,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    VkSubpassDescription subpassDescription = {
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0, NULL,
        0, NULL, NULL,
        &attachmentReference,
        0, NULL
    };
    VkRenderPass renderPass;
    VkRenderPassCreateInfo renderPassCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &subpassDescription
    };
    if (vkCreateRenderPass(device, &renderPassCreateInfo, NULL, &renderPass) != VK_SUCCESS)
    {
        printf("Failed to create render pass\n");
        return EXIT_FAILURE;
    }


    /// Create the image for storing depth.
    /// The image needs separately allocated memory.
    printf("Creating image\n");
    VkExtent3D imageExtent = {
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .depth = 1
    };
    VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_D24_UNORM_S8_UINT,
        .extent = imageExtent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queueFamilyIndex,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    VkImage image;
    VkResult code;
    if ((code = vkCreateImage(device, &imageCreateInfo, NULL, &image)) != VK_SUCCESS)
    {
        printf("Failed to create image: %s\n", resultString(code));
        return EXIT_FAILURE;
    }

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, image, &imageMemoryRequirements);

    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
    VkMemoryPropertyFlags imageMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uint32_t memoryTypeIndex;
    for (memoryTypeIndex = 0; memoryTypeIndex < physicalDeviceMemoryProperties.memoryTypeCount; ++memoryTypeIndex) {
        if (imageMemoryRequirements.memoryTypeBits & (1 << memoryTypeIndex)) {
            if ((physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & imageMemoryProperties) == imageMemoryProperties) {
                break;
            }
        }
    }
    if (memoryTypeIndex == physicalDeviceMemoryProperties.memoryTypeCount)
    {
        printf("Failed to find suitable physical device memory matching image memory requirements\n");
        return EXIT_FAILURE;
    }

    VkMemoryAllocateInfo imageAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageMemoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };
    VkDeviceMemory imageMemory;
    if (vkAllocateMemory(device, &imageAllocateInfo, NULL, &imageMemory) != VK_SUCCESS)
    {
        printf("Failed to allocate image memory\n");
        return EXIT_FAILURE;
    }
    if (vkBindImageMemory(device, image, imageMemory, 0) != VK_SUCCESS)
    {
        printf("Failed to bind image memory\n");
        return EXIT_FAILURE;
    }


    printf("Creating image view\n");
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageCreateInfo.format,
        .components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 }
    };
    VkImageView imageView;
    if (vkCreateImageView(device, &imageViewCreateInfo, NULL, &imageView) != VK_SUCCESS)
    {
        printf("Failed to create image view\n");
        return EXIT_FAILURE;
    }

    printf("Creating framebuffer\n");
    VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = 1,
        .pAttachments = &imageView,
        .width = imageExtent.width,
        .height = imageExtent.height,
        .layers = 1
    };
    VkFramebuffer framebuffer;
    if (vkCreateFramebuffer(device, &framebufferCreateInfo, NULL, &framebuffer) != VK_SUCCESS)
    {
        printf("Failed to create framebuffer\n");
        return EXIT_FAILURE;
    }


    printf("Creating vertex shader module from %s\n", VERTEX_SHADER_SOURCE_PATH);
    if (access(VERTEX_SHADER_SOURCE_PATH, F_OK))
    {
        printf("Missing vertex shader code at: %s\n", VERTEX_SHADER_SOURCE_PATH);
        return EXIT_FAILURE;
    }
    FILE* vertexShaderFile = fopen(VERTEX_SHADER_SOURCE_PATH, "r");
    fseek(vertexShaderFile, 0, SEEK_END);
    long vertexShaderCodeSize = ftell(vertexShaderFile);
    rewind(vertexShaderFile);
    uint32_t* vertexShaderCode = malloc(1 + 4 * (vertexShaderCodeSize / 4)); // multiple of 4 bytes
    fread(vertexShaderCode, 1, vertexShaderCodeSize, vertexShaderFile);
    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vertexShaderCodeSize,
        .pCode = vertexShaderCode
    };
    fclose(vertexShaderFile);
    VkShaderModule vertexShaderModule;
    if (vkCreateShaderModule(device, &vertexShaderModuleCreateInfo, NULL, &vertexShaderModule) != VK_SUCCESS)
    {
        printf("Failed to create vertex shader module\n");
        return EXIT_FAILURE;
    }
    free(vertexShaderCode);

    printf("Creating graphics pipeline\n");
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShaderModule,
            .pName = "main"
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    VkViewport viewport = {
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    VkRect2D scissor = {
        .extent = { IMAGE_WIDTH, IMAGE_HEIGHT }
    };
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f
    };
    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };
    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL, &pipelineLayout) != VK_SUCCESS)
    {
        printf("Failed to create pipeline layout\n");
        return EXIT_FAILURE;
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 1,
        .pStages = pipelineShaderStageCreateInfos,
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &pipelineRasterizationStateCreateInfo,
        .pMultisampleState = NULL,
        .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
        .layout = pipelineLayout,
        .renderPass = renderPass
    };
    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &graphicsPipeline) != VK_SUCCESS)
    {
        printf("Failed to create graphics pipeline\n");
        return EXIT_FAILURE;
    }


    /// Vulkan communicate with the device using commands send over the queue.
    /// It is inefficient to send one command at a time, so we will record the commands we want to perform in a command buffer and send it over once.
    /// Before we can create a command buffer, we need to create a command pool.
    /// The commands recorded in a command buffer must be compatible with the family of the queue they are sent over.
    /// The command pool is like a factory for command buffers, they are connected to a specific queue family on our device.
    /// Command pools also let us record command buffers in parallel in separate threads, with one pool per thread.
    printf("Creating command pool\n");
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex
    };
    VkCommandPool commandPool;
    if (vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, &commandPool) != VK_SUCCESS)
    {
        printf("Failed to create command pool\n");
        return EXIT_FAILURE;
    }

    /// With a command pool we can create a command buffer from it.
    /// To create the command buffer we specify a command pool to create a level.
    /// There are two command buffer levels in Vulkan: primary and secondary.
    /// Primary level command buffers can be submitted to queues, while secondary are called from primary commands.
    printf("Allocating command buffer\n");
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS)
    {
        printf("Failed to allocate command buffer\n");
        return EXIT_FAILURE;
    }


    /// Let us record some commands for execution into the allocated command buffer.
    /// This is the first time we are actually going "to do something", everything else up to this point is setup code.
    printf("Recording command buffer\n");
    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    VkClearValue clearValue = { .depthStencil = {1.0, 0} };
    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = framebuffer,
        .renderArea = { { 0, 0 }, { scissor.extent.width, scissor.extent.height } },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        printf("Failed to end recording of command buffer\n");
        return EXIT_FAILURE;
    }


    /// Now it is time to submit the recorded command buffer to the queue and execute the graphics pipeline.
    printf("Submitting commands to queue\n");
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        printf("Failed to submit command buffer to queue\n");
        return EXIT_FAILURE;
    }

    printf("Waiting until device is idle\n");
    vkDeviceWaitIdle(device);

    /// Finally, tear down the system by destroying objects in reverse order of creation.
    /// Before destruction of each object we will make sure it is not in use anymore.
    printf("Destroying image view\n");
    vkDestroyImageView(device, imageView, NULL);

    printf("Destroying image\n");
    vkDestroyImage(device, image, NULL);

    printf("Releasing image memory\n");
    vkFreeMemory(device, imageMemory, NULL);

    printf("Destroying vertex shader module\n");
    vkDestroyShaderModule(device, vertexShaderModule, NULL);

    printf("Releasing command buffers\n");
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    printf("Destroying command pool\n");
    vkDestroyCommandPool(device, commandPool, NULL);

    printf("Destroying pipeline\n");
    vkDestroyPipeline(device, graphicsPipeline, NULL);

    printf("Destroying pipeline layout\n");
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);

    printf("Destroying framebuffer\n");
    vkDestroyFramebuffer(device, framebuffer, NULL);

    printf("Destroying render pass\n");
    vkDestroyRenderPass(device, renderPass, NULL);

    printf("Destroying device\n");
    vkDestroyDevice(device, NULL);

    printf("Destroying instance\n");
    vkDestroyInstance(instance, NULL);

    return EXIT_SUCCESS;
}
