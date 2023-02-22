/// This is an introduction to the Vulkan core API.
/// The introduction targets an audience with background in OpenGL without previous knowledge of Vulkan.
/// For a reader belonging to this audience, Vulkan can be a bit intimidating at first glance.
/// The offical specs (https://registry.khronos.org/vulkan/specs/1.3-khr-extensions/html/) and the official tutorial (https://vulkan-tutorial.com/) are very dense.
/// In order to learn Vulkan you will need to find learning resources on the right level.
/// Below is a list of good reading material (should be read in that order).
///
///     1. Read up on how to learn Vulkan: https://www.jeremyong.com/c++/vulkan/graphics/rendering/2018/03/26/how-to-learn-vulkan/
///     2. Step through the Vulkan Guide: https://github.com/KhronosGroup/Vulkan-Guide
///
/// Then you are ready to read this document.
/// After reading through this document, I recommend reading through the classic tutorials to actually render something to screen
///
///     3. The Intel tutorial (skip the dynamic library stuff): https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-1.html
///     4. The official tutorial: https://vulkan-tutorial.com/
///
/// Certain concepts in Vulkan are not well described in tutorials but are really well described in the specification.
/// The specification is extremely dense, and is hard to read because it is aimed towards both implementors of the Vulkan API and its users at the same time.
/// The specification aims to be non-ambigiuous, not necessarily easy to understand.
/// I believe the following sections are good to read to properly understand Vulkan.
///
///     1. Execution and Memory Dependencies: https://registry.khronos.org/vulkan/specs/1.0/html/vkspec.html#synchronization-dependencies
///
/// I highly recommend to tag along and write the code yourself, no copy pasting.
/// Make sure you have a good setup with and editor and autocompletion, or your fingers will hate you.
/// I personally use Vim with CoC plugin for autocompletion using Clangd 15.
/// Spin up a web browser with some documentation
///
///     - Official 1.3 specs: https://registry.khronos.org/vulkan/specs/1.3/html
///     - Dark mode 1.0 specs: https://devdocs.io/vulkan
///
/// Install the SDK from tarball. I initially used the official vulkan packages that comes with my distro (Ubuntu 20.04 LTS), but the validation support was bad.
///
/// Also check out these tools
///
///     - https://renderdoc.org/
///     - https://gapid.dev/
///
/// Notes about the Vulkan API
///
///     1. The API makes almost no assumption about its usage.
///        This is the reason why so much code is required for so little action.
///     2. The API is decoupled. Almost every entity can be created with minimal information about other entities.
///        For example, a framebuffer can be created independently of a render pass.
///        A render pass describes dependencies between attachments in render sub passes, and a framebuffer describes which images should be used as attachments.
///        Hence, different framebuffers can be used with a single render pass, and different render passes can be used with a single framebuffer etc.
///        This makes us write very similar code over and over again, and that code needs to be compatible.
///        This might seem very error prone, but validation layers helps alot!
///
///
/// We will walk through how to setup a minimal graphics pipeline and render a triangle to image on disk.
/// The goal is to as quickly as possible see some results.
/// Despite the warnings provided in the beginning of the classic tutorials about being patient, it feel almost ridiculous for a beginner how much code is needed for setup.
/// We will not setup presentation to screen, which is one of the most demanding things to grok for a beginner. We will not need any extensions either, only the core Vulkan API.
/// As we proceed, various core Vulkan concepts will be introduced and their rationale explained.
/// We will use pure C as our language of choice because Vulkan is a C API.
/// In my opinion, mixing in a different language (usually C++) makes it harder to learn Vulkan. It is easier to learn it with a C mindset.
/// We will define the whole program in the main function.
/// Many tutorials out there are smart and factor out code into small utility functions.
/// While this is of course good practice in production code, it hampers learning for beginners, in my opinion.
/// Ok, I think we are ready to start. Happy reading!

#include <stdint.h>
#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>


/// We want to enable/disable certain features depending on the typical CMake build types (Debug/Release).
/// For example, validation layers should only be enabled in Debug.
#ifndef BUILD_TYPE
#define BUILD_TYPE ""
#endif


/// Define some user configurable compile time constants.
/// MAX_PHYSICAL_DEVICE_COUNT and MAX_PHYSICAL_DEVICE_QUEUE_FAMILIES allows us to use less dynamic allocation later on.
#define MAX_PHYSICAL_DEVICE_COUNT 4
#define MAX_PHYSICAL_DEVICE_QUEUE_FAMILIES 8
#define VERTEX_SHADER_SOURCE_PATH "out/" BUILD_TYPE "/shader.vert.spv"
#define IMAGE_WIDTH 20
#define IMAGE_HEIGHT 20


#define STR(name) #name
#define CASE_STR(name) case name : return STR(name)


/// Many functions in Vulkan return status codes.
/// We start with writing a function that converts those codes into human readable strings.
/// Taken from: https://registry.khronos.org/vulkan/specs/1.3/html/chap3.html#VkResult
const char*
resultString(VkResult code)
{
    switch (code)
    {
        CASE_STR(VK_SUCCESS);
        CASE_STR(VK_NOT_READY);
        CASE_STR(VK_TIMEOUT);
        CASE_STR(VK_EVENT_SET);
        CASE_STR(VK_EVENT_RESET);
        CASE_STR(VK_INCOMPLETE);
        CASE_STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        CASE_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        CASE_STR(VK_ERROR_INITIALIZATION_FAILED);
        CASE_STR(VK_ERROR_DEVICE_LOST);
        CASE_STR(VK_ERROR_MEMORY_MAP_FAILED);
        CASE_STR(VK_ERROR_LAYER_NOT_PRESENT);
        CASE_STR(VK_ERROR_EXTENSION_NOT_PRESENT);
        CASE_STR(VK_ERROR_FEATURE_NOT_PRESENT);
        CASE_STR(VK_ERROR_INCOMPATIBLE_DRIVER);
        CASE_STR(VK_ERROR_TOO_MANY_OBJECTS);
        CASE_STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
        CASE_STR(VK_ERROR_FRAGMENTED_POOL);
        CASE_STR(VK_ERROR_UNKNOWN);
#ifdef VK_VERSION_1_1
        CASE_STR(VK_ERROR_OUT_OF_POOL_MEMORY);
        CASE_STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
#endif
#ifdef VK_VERSION_1_2
        CASE_STR(VK_ERROR_FRAGMENTATION);
        CASE_STR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
#endif
#ifndef VK_VERSION_1_3
        CASE_STR(VK_PIPELINE_COMPILE_REQUIRED);
#endif
        default: return "UNKNOWN";
    }
}


const char*
formatString(VkFormat format) {
    switch (format) {
        CASE_STR(VK_FORMAT_D16_UNORM);
        CASE_STR(VK_FORMAT_D16_UNORM_S8_UINT);
        CASE_STR(VK_FORMAT_D24_UNORM_S8_UINT);
        CASE_STR(VK_FORMAT_D32_SFLOAT);
        CASE_STR(VK_FORMAT_D32_SFLOAT_S8_UINT);
        default: return "UNKNOWN";
    }
}


uint32_t
formatSize(VkFormat format) {
    switch (format) {
        case VK_FORMAT_D16_UNORM:          return 2;
        case VK_FORMAT_D16_UNORM_S8_UINT:  return 3;
        case VK_FORMAT_D24_UNORM_S8_UINT:  return 4;
        case VK_FORMAT_D32_SFLOAT:         return 4;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return 5;
        default: return 0;
    }
}


int main()
{
    const uint32_t imagePixelCount = IMAGE_WIDTH * IMAGE_HEIGHT;

    /// Many functions in Vulkan return a resulting status code.
    /// Sometimes we want to put the result in a variable in order to do several checks on it.
    /// For convenience we create one variable for that usage that we use throughout the whole function.
    VkResult code;


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
    /// where the destruction usually happen in reverse order of creation.
    ///
    /// For this program, we only specify the application info, which is minimal.
    /// The application info is used for things like telling Vulkan what API version we expect, and GPU vendors about our application.
    /// The latter usage can be used for application specific optimizations by a vendor, say for a game engine or game title.
    ///
    /// Note that we have to explicitly set the type of the application info structure.
    /// That seems like a common point of error, and setting this wrong indeed leads to undefined behaviour.
    /// The reason why the type exist is so that the drivers can dynamically figure out types from objects passed in.
    /// Something reserved for advanced usage.
    /// However, don't be afraid: validation layers in debug mode will detect this, so in practice this is not really an issue (as long as you excercise all code paths ofc).
    /// We use a compile time flag to select wheter we should enable validation layers or not.
    /// There exist many validation layers, we only use the code Krhonos validation layer, which does conformance checking against the API.
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
    ///   - The CPU with a software implementation of Vulkan called Lavapipe
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
    uint32_t physicalDeviceCount = MAX_PHYSICAL_DEVICE_COUNT;
    VkPhysicalDevice physicalDevices[MAX_PHYSICAL_DEVICE_COUNT];
    if ((code = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices)) != VK_SUCCESS)
    {
        if (code == VK_INCOMPLETE) {
            printf("There are more than MAX_PHYSICAL_DEVICE_COUNT physical devices available, consider recompiling with a different value\n");
        }
        else {
            printf("Failed to enumerate physical devices, code: %d\n", code);
            return EXIT_FAILURE;
        }
    }
    printf("%d physical devices available\n", physicalDeviceCount);
    if (physicalDeviceCount == 0)
    {
        printf("Found no physical device\n");
        return EXIT_FAILURE;
    }


    /// We managed to enumerate all physical devices, now it is time to pick the most suitable one.
    /// We want to know the index of the physical device among all physical devices.
    /// We also want to know the queue family index for that physical device.
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

        VkQueueFamilyProperties queueFamilyProperties[MAX_PHYSICAL_DEVICE_QUEUE_FAMILIES];
        uint32_t queueFamiliesCount = MAX_PHYSICAL_DEVICE_QUEUE_FAMILIES;
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
    /// In advanced setups, logical devices can encompass several physical devices (assuming they belong to the same device group that can share memory and queues etc).
    /// We need to specify a queue priority, which is arbitrarily set to 1 since we are only going to use one queue.
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


    /// That is the core setup code that probably needs to be done for all Vulkan programs.
    /// Next step is to allocate resources for the image we will render to, as well as a pixel readback buffer.
    /// Vulkan distinguish images, buffers, memory and views into those from each other.
    /// In Vulkan, memory can be allocated on different physical devices, on different heaps of different memory types.
    /// The memory type becomes important when you want to transfer data between device and host, for example.
    /// You will never (to my knowledge) operate directly on memory, but through buffers and images or other memory like objects.
    /// Buffers are simple memory objects. They add the functionality of belonging to a queue, having a usage flag etc.
    /// Several buffers can share memory (they can overlap, for example), which also should highlight why it is good to differ between raw memory and the buffer that lies on top of it.
    /// Images are more advanced than buffers.
    /// Buffers represent linear memory, while images support several representations optimized for graphics such as formats, mipmaps, layers, multisampling.
    /// Images can also be (and usually are) tiled, which makes them more efficient than buffers.
    /// Images also have something called a layout, which specifies what kind of operation they are optimized for.
    /// You want to specify a certain layout when rendering, and then transition it to another before transferring.
    /// Finally, you can create views, which specify a subset of the underlying resource to access.
    /// This is what eventually will go into the framebuffer.
    ///
    /// Ok, what resources do we need?
    /// We need an image + image memory + image view for the render target.
    /// We will also need a buffer that we can transfer the image to after rendering to it.
    /// Having the rendered content in a buffer allows us to memory map it and copy back to the host.

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
    if ((code = vkCreateImage(device, &imageCreateInfo, NULL, &image)) != VK_SUCCESS)
    {
        printf("Failed to create image: %s\n", resultString(code));
        return EXIT_FAILURE;
    }

    /// With an image object we can query for which memory type we want to use for it.
    /// Every image can be queried for its memory requirements, which we then can compare with the memory properties provided by the physical device.
    /// We created the image using the device, so it knows about what memory types are available.
    /// The memory types that the image can access is provided by a bitmask.
    /// If the bit at position `i` is set means that memory type `i` is compatible with the image memory requirements.
    /// This leads to some bit-shifting logic beneath.
    /// We require that the image memory have the HOST_VISIBLE and HOST_COHERENT bits set.
    /// HOST_VISIBLE means that the memory can be mapped to host memory
    /// HOST_COHERENT means that device writes to the memory will be visible to the host without extra flushing commands.
    /// Note the slight inconsistency in the naming conventions here.
    /// Memory visibility is a concept in Vulkan related to synchronization of commands, which is what the HOST_COHERENT bit addresses.
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

    printf("Allocating image memory\n");
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

    printf("Binding image memory\n");
    if (vkBindImageMemory(device, image, imageMemory, 0) != VK_SUCCESS)
    {
        printf("Failed to bind image to image memory\n");
        return EXIT_FAILURE;
    }

    /// We create an image view by specifying which mip level and array layer we want to access.
    /// We also specify which "aspects" of an image we want to access.
    /// In our case, we are want to view both the depth and the stencil part of the image, so we or those to apsect together.
    /// Note that we need to specify that we want a 2D image view again.
    /// The component mapping can be used to "swizzle" around the components of each pixel. Usually this is assigned a 4-tuple of "swizzle identity".
    /// Setting the format to something different than the format of the image can be used to reinterpret the image components.
    printf("Creating image view\n");
    VkImageSubresourceRange imageSubresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageCreateInfo.format,
        .components = { VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = imageSubresourceRange
    };
    VkImageView imageView;
    if (vkCreateImageView(device, &imageViewCreateInfo, NULL, &imageView) != VK_SUCCESS)
    {
        printf("Failed to create image view\n");
        return EXIT_FAILURE;
    }


    /// Now we have defined the image, memory and view for the render target.
    /// We also need to create a buffer which we can read back the rendered data to the host with.
    /// The procedure for allocating a suitable memory for the buffer is similar to the one for the image.
    printf("Creating image pixel read back buffer\n");
    VkBuffer imageBuffer;
    VkDeviceSize imageBufferSize = formatSize(imageCreateInfo.format) * IMAGE_WIDTH * IMAGE_HEIGHT;
    if (imageBufferSize == 0)
    {
        printf("Failed to estimate byte size of image format: %s\n", formatString(imageCreateInfo.format));
        return EXIT_FAILURE;
    }
    VkBufferCreateInfo imageBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = imageBufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queueFamilyIndex
    };
    if (vkCreateBuffer(device, &imageBufferCreateInfo, NULL, &imageBuffer) != VK_SUCCESS)
    {
        printf("Failed to create image buffer\n");
        return EXIT_FAILURE;
    }

    VkMemoryRequirements imageBufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, imageBuffer, &imageBufferMemoryRequirements);
    VkMemoryPropertyFlags imageBufferMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (memoryTypeIndex = 0; memoryTypeIndex < physicalDeviceMemoryProperties.memoryTypeCount; ++memoryTypeIndex) {
        if (imageBufferMemoryRequirements.memoryTypeBits & (1 << memoryTypeIndex)) {
            if ((physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & imageBufferMemoryProperties) == imageBufferMemoryProperties) {
                break;
            }
        }
    }
    if (memoryTypeIndex == physicalDeviceMemoryProperties.memoryTypeCount)
    {
        printf("Failed to find suitable physical device memory matching image buffer memory requirements\n");
        return EXIT_FAILURE;
    }

    printf("Allocating image buffer memory\n");
    VkMemoryAllocateInfo imageBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageBufferSize,
        .memoryTypeIndex = memoryTypeIndex
    };
    VkDeviceMemory imageBufferMemory;
    if (vkAllocateMemory(device, &imageBufferAllocateInfo, NULL, &imageBufferMemory) != VK_SUCCESS)
    {
        printf("Failed to allocated image buffer memory\n");
        return EXIT_FAILURE;
    }

    printf("Binding image buffer to image buffer memory\n");
    if (vkBindBufferMemory(device, imageBuffer, imageBufferMemory, 0) != VK_SUCCESS)
    {
        printf("Failed to bind image buffer to image buffer memory\n");
        return EXIT_FAILURE;
    }



    /// In order to render something, we need to define a render pass, a framebuffer, shader modules and a graphics pipeline.
    /// A render pass describes how stuff will be rendered.
    /// A framebuffer describes which images the render pass will render into.
    /// A graphics pipeline put all this together.
    printf("Creating render pass\n");
    VkAttachmentDescription attachmentDescription = {
        .flags = 0,
        .format = VK_FORMAT_D24_UNORM_S8_UINT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    VkAttachmentReference attachmentReference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    VkSubpassDescription subpassDescription = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pDepthStencilAttachment = &attachmentReference,
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
    size_t vertexShaderCodeSize = ftell(vertexShaderFile);
    rewind(vertexShaderFile);
    uint32_t* vertexShaderCode = (uint32_t*) malloc(1 + 4 * (vertexShaderCodeSize / 4)); // multiple of 4 bytes
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
    /// Using a command pool also makes allocating new command buffers more efficient that it would be allocating them in isolation.
    printf("Creating command pool\n");
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
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
    /// Primary level command buffers can be submitted to queues, while secondary are called from primary commands (advanced usage).
    /// When the command buffer is allocated, it is put into "initial state".
    /// Operations on command buffers act like a state machine and transitions the command buffer state.
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
    /// This will put the command buffer into "recording state".
    /// There exist several families of commands that can be recorded in a command buffer: action, state, synchronization and launch commands.
    /// TODO: Set optimal flags
    printf("Recording command buffer\n");
    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    VkClearValue clearValue = { .depthStencil = {1.0f, 0} };
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

    /// Efter the render pass we want change the depth buffer image layout.
    /// We do that using an image memory barrier to synchronize access before and after the layout transition.
    /// Note that this can also be expressed using render subpass dependencies.
    /// This is probably more efficient if we are using more than one subpass.
    VkImageMemoryBarrier imageMemoryBarrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        // .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        // .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = queueFamilyIndex,
        .dstQueueFamilyIndex = queueFamilyIndex,
        .image = image,
        .subresourceRange = imageSubresourceRange
    };
    /// Can also use VkDependencyInfo + vkCmdPipelineBarrier2
    /// We specify that the execution and memory dependencies are "framebuffer local" by setting the VK_DEPENDENCY_BY_REGION_BIT.
    /// This allows Vulkan to perform more aggressive optimizations.
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT,
                         0, NULL,
                         0, NULL,
                         1, &imageMemoryBarrier);
    VkBufferImageCopy imageRegion = {
        .imageSubresource = {
            .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
            .mipLevel       = imageSubresourceRange.baseMipLevel,
            .baseArrayLayer = imageSubresourceRange.baseArrayLayer,
            .layerCount     = imageSubresourceRange.levelCount
        },
        .imageExtent = imageExtent
    };
    vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageBuffer, 1, &imageRegion);

    /// Finish the recoding of the command buffer.
    /// This will put the command buffer into "executable state", that is, we can submit it for execution.
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        printf("Failed to end recording of command buffer\n");
        return EXIT_FAILURE;
    }

    /// Now it is time to submit the recorded command buffer to the queue and execute the graphics pipeline.
    /// Submitting the command buffer will put it into "pending state".
    /// Depending on how the command buffer was created, it will be put back into either "executable" or "invalid" state upon completion.
    /// Note that you can't check the state of the command buffer, in particular there is no "executing" state.
    /// We will also create a fence object so that we know when the command has finished executing.
    /// The way we use the fence here is equivalent to using vkQueueWaitIdle, but we use fences here for demonstration purposes.
    /// When creating the device we made sure to get a queue from a family supporting both graphics and transfer operations.
    /// A more efficient and portable solutions is to get two separate queues and synchronize them using semaphores.
    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    VkFence fence;
    if (vkCreateFence(device, &fenceCreateInfo, NULL, &fence) != VK_SUCCESS)
    {
        printf("Failed to create fence\n");
        return EXIT_FAILURE;
    }
    printf("Submitting commands to queue\n");
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS)
    {
        printf("Failed to submit command buffer to queue\n");
        return EXIT_FAILURE;
    }

    while ((code = vkWaitForFences(device, 1, &fence, VK_TRUE, 1000000)) != VK_SUCCESS) {
        printf("Waiting until fence is signaled, current status: %s\n", resultString(code));
    }

    printf("Command execution completed! Reading back pixels to host\n");
    void* mappedImageBufferMemory;
    uint32_t* imageData = (uint32_t*) malloc(imageBufferCreateInfo.size);
    vkMapMemory(device, imageBufferMemory, 0, imageBufferCreateInfo.size, 0, &mappedImageBufferMemory);
    memcpy(imageData, mappedImageBufferMemory, imageBufferCreateInfo.size);
    vkUnmapMemory(device, imageBufferMemory);

    float* depthData = (float*) malloc(imagePixelCount * sizeof(float));
    for (uint32_t i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; ++i) {
        uint32_t unormDepth = imageData[i] >> 1;
        depthData[i] = ((float) unormDepth) / ((1 << 23) - 1);
        if (unormDepth  == ((1<<23) - 1)) {
            depthData[i] = 0;
        }
    }
    free(imageData);

    FILE* outputFile = fopen("out.dat", "w");
    for (uint32_t i = 0; i < IMAGE_WIDTH; ++i) {
        for (uint32_t j = 0; j < IMAGE_HEIGHT; ++j) {
            fprintf(outputFile, "%.4f ", depthData[IMAGE_WIDTH * i + j]);
        }
        fprintf(outputFile, "\n");
    }
    fclose(outputFile);
    free(depthData);

    /// Finally, tear down the system.
    /// Before destruction of each object we need to make sure it is not in use anymore, which is easiest by waiting for the queue to become idle.
    /// All resources that are childs of another resource needs to be released before their parent.
    /// The easiest way to do this is by destroying objects in reverse order of creation.
    /// Resources allocated from pools do not have to be manually freed, but we will do it anyways to show how it can be done manually.
    printf("Waiting until device is idle\n");
    vkDeviceWaitIdle(device);

    printf("Destroying fence\n");
    vkDestroyFence(device, fence, NULL);

    printf("Destroying image buffer\n");
    vkDestroyBuffer(device, imageBuffer, NULL);

    printf("Destroying image buffer memory\n");
    vkFreeMemory(device, imageBufferMemory, NULL);

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
