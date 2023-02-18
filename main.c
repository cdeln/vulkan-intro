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

#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>


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
    printf("Creating instance\n");
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_0
    };
    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo
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
        return EXIT_SUCCESS;
    }

    printf("Selecting a suitable physical device\n");
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    uint32_t physicalDeviceIndex;
    uint32_t queueFamilyIndex;
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


    /// Finally, tear down the system by destroying objects in reverse order of creation.
    /// Before destruction of each object we will make sure it is not in use anymore.
    printf("Destroying device\n");
    vkDeviceWaitIdle(device);
    vkDestroyDevice(device, NULL);

    printf("Destroying instance\n");
    vkDestroyInstance(instance, NULL);

    return EXIT_SUCCESS;
}
