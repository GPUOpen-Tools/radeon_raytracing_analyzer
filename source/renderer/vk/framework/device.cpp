//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Vulkan device object.
//=============================================================================

#include <cassert>
#include <string>
#include <stdexcept>
#include <string.h>

#include <volk/volk.h>

#define VMA_IMPLEMENTATION

#include "device.h"
#include "instance.h"
#include "instance_properties.h"
#include "device_properties.h"
#include "ext_debug_utils.h"
#include "ext_validation.h"
#include "ext_gpu_validation.h"

#include "../util_vulkan.h"

#include "public/rra_print.h"

namespace rra
{
    namespace renderer
    {
        bool Device::OnCreate(const char*       app_name,
                              const char*       engine_name,
                              bool              cpu_validation_enabled,
                              bool              gpu_validation_enabled,
                              const WindowInfo* window_info)
        {
            bool result = false;

            VkResult volk_init_result = volkInitialize();
            CheckResult(volk_init_result, "Failed to initialize Vulkan loader.");
            result = volk_init_result == VK_SUCCESS;
            if (result)
            {
                // Configure the list of instance properties to set during instance creation.
                InstanceProperties instance_properties = {};

                result = instance_properties.Initialize() == VK_SUCCESS;
                if (result)
                {
                    SetEssentialInstanceExtensions(cpu_validation_enabled, gpu_validation_enabled, &instance_properties);

                    // Create the Vulkan instance.
                    VkInstance       vulkan_instance = VK_NULL_HANDLE;
                    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
                    result = result && (CreateInstance(app_name, engine_name, &vulkan_instance, &physical_device, &instance_properties) == VK_SUCCESS);

                    if (result)
                    {
                        // Configure the list of device properties to set during device creation.
                        DeviceProperties device_properties = {};
                        device_properties.Initialize(physical_device);
                        SetEssentialDeviceExtensions(&device_properties);

                        // Create the device.
                        OnCreateEx(vulkan_instance, physical_device, &device_properties, window_info);

                        // Do we have a valid device ready to use?
                        result = physical_device_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE;
                        if (result)
                        {
                            VmaAllocatorCreateInfo allocator_info = {};
                            allocator_info.vulkanApiVersion       = VK_API_VERSION_1_2;
                            allocator_info.physicalDevice         = GetPhysicalDevice();
                            allocator_info.device                 = GetDevice();
                            allocator_info.instance               = GetInstance();

                            // Attempt to create the memory allocator.
                            VkResult alloc_init_result = vmaCreateAllocator(&allocator_info, &allocator_);
                            CheckResult(alloc_init_result, "Failed to create VMA allocator.");

                            result = alloc_init_result == VK_SUCCESS;
                        }
                    }
                }
            }

            return result;
        }

        void Device::SetEssentialInstanceExtensions(bool cpu_validation_enabled, bool gpu_validation_enabled, InstanceProperties* instance_properties)
        {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            instance_properties->AddInstanceExtensionName(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            instance_properties->AddInstanceExtensionName(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
            instance_properties->AddInstanceExtensionName(VK_KHR_SURFACE_EXTENSION_NAME);

            ExtDebugUtilsCheckInstanceExtensions(instance_properties);

            if (cpu_validation_enabled)
            {
                ExtDebugReportCheckInstanceExtensions(instance_properties);
            }

            if (gpu_validation_enabled)
            {
                ExtGPUValidationCheckExtensions(instance_properties);
            }
        }

        void Device::SetEssentialDeviceExtensions(DeviceProperties* device_properties)
        {
            device_properties->AddDeviceExtensionName(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            device_properties->AddDeviceExtensionName(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
            device_properties->AddDeviceExtensionName(VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME);
            device_properties->AddDeviceExtensionName(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
        }

        void Device::OnCreateEx(VkInstance instance, VkPhysicalDevice physical_device, DeviceProperties* device_properties, const WindowInfo* window_info)
        {
            VkResult result;

            instance_        = instance;
            physical_device_ = physical_device;

            // Get queue/memory/device properties.
            uint32_t queue_family_count;
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);
            assert(queue_family_count >= 1);

            std::vector<VkQueueFamilyProperties> queue_props;
            queue_props.resize(queue_family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_props.data());
            assert(queue_family_count >= 1);

            // Query the device properties.
            vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties_);
            vkGetPhysicalDeviceProperties(physical_device_, &device_properties_);

            // Get subgroup properties to check if subgroup operations are supported.
            subgroup_properties_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
            subgroup_properties_.pNext = nullptr;

            device_properties_2_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            device_properties_2_.pNext = &subgroup_properties_;

            vkGetPhysicalDeviceProperties2(physical_device_, &device_properties_2_);

#if defined(_WIN32)
            // Create a Win32 Surface.
            VkWin32SurfaceCreateInfoKHR surface_create_info = {};
            surface_create_info.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surface_create_info.pNext                       = nullptr;
            surface_create_info.hinstance                   = nullptr;
            surface_create_info.hwnd                        = window_info->window_handle;
            result                                          = vkCreateWin32SurfaceKHR(instance_, &surface_create_info, nullptr, &surface_);
            CheckResult(result, "Could not create Win32 surface.");
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            // Create the XCB surface.
            VkXcbSurfaceCreateInfoKHR surface_create_info = {};
            surface_create_info.sType                     = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            surface_create_info.pNext                     = nullptr;
            surface_create_info.flags                     = 0;
            surface_create_info.connection                = window_info->connection;
            surface_create_info.window                    = window_info->window;
            result                                        = vkCreateXcbSurfaceKHR(instance_, &surface_create_info, nullptr, &surface_);
            CheckResult(result, "Could not create XCB surface.");
#endif
            // Find a graphics device and a queue that can present to the above surface.
            graphics_queue_family_index_ = UINT32_MAX;
            present_queue_family_index_  = UINT32_MAX;
            for (uint32_t family_index = 0; family_index < queue_family_count; ++family_index)
            {
                if ((queue_props[family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
                {
                    if (graphics_queue_family_index_ == UINT32_MAX)
                        graphics_queue_family_index_ = family_index;

                    VkBool32 supports_present;
                    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, family_index, surface_, &supports_present);
                    if (supports_present == VK_TRUE)
                    {
                        graphics_queue_family_index_ = family_index;
                        present_queue_family_index_  = family_index;
                        break;
                    }
                }
            }

            // If didn't find a queue that supports both graphics and present, then find a separate present queue.
            if (present_queue_family_index_ == UINT32_MAX)
            {
                for (uint32_t family_index = 0; family_index < queue_family_count; ++family_index)
                {
                    VkBool32 supports_present;
                    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, family_index, surface_, &supports_present);
                    if (supports_present == VK_TRUE)
                    {
                        present_queue_family_index_ = family_index;
                        break;
                    }
                }
            }

            compute_queue_family_index_ = UINT32_MAX;

            for (uint32_t family_index = 0; family_index < queue_family_count; ++family_index)
            {
                if ((queue_props[family_index].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
                {
                    if (compute_queue_family_index_ == UINT32_MAX)
                        compute_queue_family_index_ = family_index;
                    if (family_index != graphics_queue_family_index_)
                    {
                        compute_queue_family_index_ = family_index;
                        break;
                    }
                }
            }

            // Prepare existing extensions names into a buffer for vkCreateDevice.
            std::vector<const char*> extension_names;
            device_properties->GetExtensionNamesAndConfigs(extension_names);

            // Create the logical device.
            float queue_priorities[1] = {0.0};

            VkDeviceQueueCreateInfo queue_info[2] = {};
            queue_info[0].sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[0].pNext                   = nullptr;
            queue_info[0].queueCount              = 1;
            queue_info[0].pQueuePriorities        = queue_priorities;
            queue_info[0].queueFamilyIndex        = graphics_queue_family_index_;

            queue_info[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[1].pNext            = nullptr;
            queue_info[1].queueCount       = 1;
            queue_info[1].pQueuePriorities = queue_priorities;
            queue_info[1].queueFamilyIndex = compute_queue_family_index_;

            VkPhysicalDeviceFeatures physical_device_features       = {};
            physical_device_features.fillModeNonSolid               = true;
            physical_device_features.pipelineStatisticsQuery        = true;
            physical_device_features.fragmentStoresAndAtomics       = true;
            physical_device_features.vertexPipelineStoresAndAtomics = true;
            physical_device_features.shaderImageGatherExtended      = true;
            physical_device_features.wideLines                      = true;  // Needed for drawing lines with a specific width.
            physical_device_features.shaderInt64                    = true;

            VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features = {};
            extended_dynamic_state_features.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
            extended_dynamic_state_features.extendedDynamicState = VK_TRUE;

            VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {};
            descriptor_indexing_features.pNext                                      = &extended_dynamic_state_features;
            descriptor_indexing_features.sType                                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
            descriptor_indexing_features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
            descriptor_indexing_features.runtimeDescriptorArray                     = VK_TRUE;

            VkPhysicalDeviceFeatures2 physical_device_features_2 = {};
            physical_device_features_2.sType                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physical_device_features_2.features                  = physical_device_features;
            physical_device_features_2.pNext                     = &descriptor_indexing_features;

            VkDeviceCreateInfo device_info      = {};
            device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_info.pNext                   = &physical_device_features_2;
            device_info.queueCreateInfoCount    = 2;
            device_info.pQueueCreateInfos       = queue_info;
            device_info.enabledExtensionCount   = static_cast<uint32_t>(extension_names.size());
            device_info.ppEnabledExtensionNames = device_info.enabledExtensionCount ? extension_names.data() : nullptr;
            device_info.pEnabledFeatures        = nullptr;

            RraPrint("Device enabled extension names:");
            for (auto device_extension_name : extension_names)
            {
                RraPrint("\t%s", device_extension_name);
            }

            result = vkCreateDevice(physical_device_, &device_info, nullptr, &device_);
            CheckResult(result, "Failed to create device.");

            // Create the device queues.
            vkGetDeviceQueue(device_, graphics_queue_family_index_, 0, &graphics_queue_);
            if (graphics_queue_family_index_ == present_queue_family_index_)
            {
                present_queue_ = graphics_queue_;
            }
            else
            {
                vkGetDeviceQueue(device_, present_queue_family_index_, 0, &present_queue_);
            }

            if (compute_queue_family_index_ != UINT32_MAX)
            {
                vkGetDeviceQueue(device_, compute_queue_family_index_, 0, &compute_queue_);
            }

            // Initialize the extensions (if they have been enabled successfully).
            ExtDebugUtilsGetProcAddresses(device_);
        }

        VkDevice Device::GetDevice() const
        {
            return device_;
        }

        VkQueue Device::GetGraphicsQueue() const
        {
            return graphics_queue_;
        }

        uint32_t Device::GetGraphicsQueueFamilyIndex() const
        {
            return present_queue_family_index_;
        }

        VkQueue Device::GetPresentQueue() const
        {
            return present_queue_;
        }

        uint32_t Device::GetPresentQueueFamilyIndex() const
        {
            return graphics_queue_family_index_;
        }

        VkQueue Device::GetComputeQueue() const
        {
            return compute_queue_;
        }

        uint32_t Device::GetComputeQueueFamilyIndex() const
        {
            return compute_queue_family_index_;
        }

        VkPhysicalDevice Device::GetPhysicalDevice() const
        {
            return physical_device_;
        }

        VkPhysicalDeviceMemoryProperties Device::GetPhysicalDeviceMemoryProperties() const
        {
            return memory_properties_;
        }

        VkPhysicalDeviceProperties Device::GetPhysicalDeviceProperties() const
        {
            return device_properties_;
        }

        VkPhysicalDeviceSubgroupProperties Device::GetPhysicalDeviceSubgroupProperties() const
        {
            return subgroup_properties_;
        }

        VkInstance Device::GetInstance() const
        {
            return instance_;
        }

        VmaAllocator Device::GetAllocator() const
        {
            return allocator_;
        }

        void Device::OnDestroy()
        {
            if (instance_ == VK_NULL_HANDLE)
            {
                return;
            }

            GPUFlush();

            if (allocator_ != VK_NULL_HANDLE)
            {
                vmaDestroyAllocator(allocator_);
                allocator_ = VK_NULL_HANDLE;
            }

            if (surface_ != VK_NULL_HANDLE)
            {
                vkDestroySurfaceKHR(instance_, surface_, nullptr);
                surface_ = VK_NULL_HANDLE;
            }

            if (device_ != VK_NULL_HANDLE)
            {
                vkDestroyDevice(device_, nullptr);
                device_ = VK_NULL_HANDLE;
            }

            ExtDebugReportOnDestroy(instance_);

            DestroyInstance(instance_);

            instance_ = VK_NULL_HANDLE;
        }

        void Device::GPUFlush()
        {
            if (device_ != VK_NULL_HANDLE)
            {
                vkDeviceWaitIdle(device_);
            }
        }

        void Device::FlushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, VkCommandPool pool, bool free)
        {
            if (command_buffer == VK_NULL_HANDLE)
            {
                return;
            }

            VkResult end_result = vkEndCommandBuffer(command_buffer);
            CheckResult(end_result, "Failed to end command buffer.");

            VkSubmitInfo submit_info       = {};
            submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &command_buffer;

            // Create fence to ensure that the command buffer has finished executing.
            VkFenceCreateInfo fence_info = {};
            fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

            VkFence  fence         = VK_NULL_HANDLE;
            VkResult create_result = vkCreateFence(device_, &fence_info, nullptr, &fence);
            CheckResult(create_result, "Failed to create command buffer completion fence.");

            // Submit to the queue.
            VkResult submit_result = vkQueueSubmit(queue, 1, &submit_info, fence);
            CheckResult(submit_result, "Failed to submit command buffer.");

            // Wait for the fence to signal that command buffer has finished executing.
            VkResult wait_result = vkWaitForFences(device_, 1, &fence, VK_TRUE, 100000000000);
            CheckResult(wait_result, "Failed to wait for submission fence.");

            vkDestroyFence(device_, fence, nullptr);

            if (free)
            {
                vkFreeCommandBuffers(device_, pool, 1, &command_buffer);
            }
        }

        void Device::CreateBuffer(VkBufferUsageFlags usage_flags,
                                  VmaMemoryUsage     memory_usage,
                                  VkBuffer&          buffer,
                                  VmaAllocation&     allocation,
                                  const void*        data,
                                  VkDeviceSize       size)
        {
            RRA_ASSERT(buffer == VK_NULL_HANDLE);

            VkBufferCreateInfo buffer_info = {};
            buffer_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_info.size               = size;
            buffer_info.usage              = usage_flags;

            VmaAllocationCreateInfo alloc_info = {};
            alloc_info.usage                   = memory_usage;

            VkResult result = vmaCreateBuffer(allocator_, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
            CheckResult(result, "Failed to create buffer.");

            buffer_allocation_count_++;

            if (result == VK_SUCCESS)
            {
                // We will only run into this case during the initial loading. CheckResult will catch others during rendering.
                WriteToBuffer(allocation, data, size);
            }
        }

        void Device::TransferBufferToDevice(VkCommandBuffer cmd, VkQueue queue, VkBuffer buffer, const void* data, VkDeviceSize size)
        {
            VkBuffer      staging_buffer{};
            VmaAllocation staging_allocation{};

            CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, staging_buffer, staging_allocation, data, size);

            vkResetCommandBuffer(cmd, 0);

            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags            = 0;
            begin_info.pInheritanceInfo = nullptr;

            VkResult result = vkBeginCommandBuffer(cmd, &begin_info);
            CheckResult(result, "Failed to begin command buffer.");

            VkBufferCopy buffer_copy{};
            buffer_copy.srcOffset = 0;
            buffer_copy.dstOffset = 0;
            buffer_copy.size      = size;

            vkCmdCopyBuffer(cmd, staging_buffer, buffer, 1, &buffer_copy);

            FlushCommandBuffer(cmd, queue, VK_NULL_HANDLE, false);

            vmaDestroyBuffer(allocator_, staging_buffer, staging_allocation);
        }

        void Device::CreateImage(VkImageCreateInfo image_create_info, VmaMemoryUsage memory_usage, VkImage& image, VmaAllocation& allocation)
        {
            RRA_ASSERT(image == VK_NULL_HANDLE);

            VmaAllocationCreateInfo alloc_info = {};
            alloc_info.usage                   = memory_usage;

            VkResult result = vmaCreateImage(allocator_, &image_create_info, &alloc_info, &image, &allocation, nullptr);
            CheckResult(result, "Failed to create image.");

            image_allocation_count_++;
        }

        void Device::WriteToBuffer(const VmaAllocation& allocation, const void* data, VkDeviceSize size)
        {
            if (data)
            {
                void*    mapped_data;
                VkResult result = vmaMapMemory(allocator_, allocation, &mapped_data);
                CheckResult(result, "Failed to map memory.");

                if (result == VK_SUCCESS)
                {
                    // We will only run into this case during the initial loading. CheckResult will catch others during rendering.
                    memcpy(mapped_data, data, size);
                    vmaUnmapMemory(allocator_, allocation);
                }
            }
        }

        void Device::ZeroOutBuffer(const VmaAllocation& allocation, VkDeviceSize size)
        {
            void*    mapped_data;
            VkResult result = vmaMapMemory(allocator_, allocation, &mapped_data);
            CheckResult(result, "Failed to map memory.");

            if (result == VK_SUCCESS)
            {
                memset(mapped_data, 0, size);
                vmaUnmapMemory(allocator_, allocation);
            }
        }

        void Device::ReadFromBuffer(const VmaAllocation& allocation, void* data, VkDeviceSize size)
        {
            void*    mapped_data;
            VkResult result = vmaMapMemory(allocator_, allocation, &mapped_data);
            CheckResult(result, "Failed to map memory.");

            if (result == VK_SUCCESS)
            {
                // We will only run into this case during the initial loading. CheckResult will catch others during rendering.
                memcpy(data, mapped_data, size);
                vmaUnmapMemory(allocator_, allocation);
            }
        }

        void Device::DestroyBuffer(VkBuffer& buffer, VmaAllocation& allocation)
        {
            if (allocator_ == VK_NULL_HANDLE)
            {
                return;
            }
            if (buffer != VK_NULL_HANDLE)
            {
                buffer_allocation_count_--;
            }
            vmaDestroyBuffer(allocator_, buffer, allocation);
            buffer     = VK_NULL_HANDLE;
            allocation = VK_NULL_HANDLE;
        }

        void Device::DestroyImage(VkImage& image, VmaAllocation& allocation)
        {
            if (allocator_ == VK_NULL_HANDLE)
            {
                return;
            }
            if (image != VK_NULL_HANDLE)
            {
                image_allocation_count_--;
            }
            vmaDestroyImage(allocator_, image, allocation);
            image      = VK_NULL_HANDLE;
            allocation = VK_NULL_HANDLE;
        }

        std::vector<VkSampleCountFlagBits> Device::GetPossibleMSAASampleSettings()
        {
            auto                  device_properties = GetPhysicalDeviceProperties();
            VkSampleCountFlags    max_counts = device_properties.limits.framebufferColorSampleCounts & device_properties.limits.framebufferDepthSampleCounts;
            VkSampleCountFlagBits available_count = VK_SAMPLE_COUNT_64_BIT;

            std::vector<VkSampleCountFlagBits> possible_settings;

            while (available_count > 0)
            {
                if (available_count <= static_cast<VkSampleCountFlagBits>(max_counts))
                {
                    possible_settings.push_back(available_count);
                }
                available_count = static_cast<VkSampleCountFlagBits>(available_count / 2);
            }

            return possible_settings;
        }
    }  // namespace renderer
}  // namespace rra
