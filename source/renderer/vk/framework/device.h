//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Vulkan device object.
//=============================================================================

#ifndef RRA_RENDERER_VK_FRAMEWORK_DEVICE_H_
#define RRA_RENDERER_VK_FRAMEWORK_DEVICE_H_

#include <string>

#include <volk/volk.h>

#include "device_properties.h"
#include "instance_properties.h"

#include "../../public/renderer_types.h"
#include "../util_vulkan.h"

#include "../../public/include_vma.h"

namespace rra
{
    namespace renderer
    {
        /// @brief The Device type declaration.
        ///
        /// The device can be used to create resource and submit rendering commands for execution on GPU hardware.
        class Device
        {
        public:
            /// @brief Constructor.
            Device() = default;

            /// @brief Destructor.
            ~Device() = default;

            /// @brief Initialization of the device.
            ///
            /// @param [in] app_name The application name string.
            /// @param [in] engine_name The engine name string.
            /// @param [in] cpu_validation_enabled When set to true CPU validation will be enabled for device calls.
            /// @param [in] gpu_validation_enabled When set to true GPU validation will be enabled for device calls.
            /// @param [in] window_info The info for the window where the framebuffer will be presented.
            /// @returns True if the device was created successfully, and false in case of failure.
            bool OnCreate(const char*       app_name,
                          const char*       engine_name,
                          bool              cpu_validation_enabled,
                          bool              gpu_validation_enabled,
                          const WindowInfo* window_info);

            /// @brief Initialize the set of required instance extension properties with essential validation layer name strings.
            ///
            /// @param [in] cpu_validation_enabled When set to true, specific CPU validation layers will be added as an instance extension.
            /// @param [in] gpu_validation_enabled When set to true, specific GPU validation layers will be added as an instance extension.
            /// @param [in/out] instance_properties The set of instance layer properties that will be applied at instance creation time.
            void SetEssentialInstanceExtensions(bool cpu_validation_enabled, bool gpu_validation_enabled, InstanceProperties* instance_properties);

            /// @brief Initialize the set of required device extensions to configure upon device creation.
            ///
            /// @param [in] device_properties The set of required device layer properties to configure upon device creation.
            void SetEssentialDeviceExtensions(DeviceProperties* device_properties);

            /// @brief Initialize the device using pre-existing instance and physical device handles.
            ///
            /// @param [in] instance The Vulkan instance used to create the device.
            /// @param [in] physical_device The underlying physical device to create a logical device for.
            /// @param [in] device_properties The device properties to use when creating the logical device.
            /// @param [in] window_info The info for the window where the framebuffer will be presented.
            void OnCreateEx(VkInstance instance, VkPhysicalDevice physical_device, DeviceProperties* device_properties, const WindowInfo* window_info);

            /// @brief Destroy the Vulkan device and instance.
            void OnDestroy();

            /// @brief Retrieve a handle to the device.
            ///
            /// @returns The device handle.
            VkDevice GetDevice() const;

            /// @brief Retrieve the handle to the device graphics queue.
            ///
            /// @returns The handle to the device graphics queue.
            VkQueue GetGraphicsQueue() const;

            /// @brief Retrieve the handle to the device graphics queue family index.
            ///
            /// @returns The handle to the device graphics queue family index.
            uint32_t GetGraphicsQueueFamilyIndex() const;

            /// @brief Retrieve the handle to the device present queue.
            ///
            /// @returns The handle to the device present queue.
            VkQueue GetPresentQueue() const;

            /// @brief Retrieve the handle to the device present queue family index.
            ///
            /// @returns The handle to the device graphics queue family index.
            uint32_t GetPresentQueueFamilyIndex() const;

            /// @brief Retrieve the handle to the device compute queue.
            ///
            /// @returns The handle to the device compute queue.
            VkQueue GetComputeQueue() const;

            /// @brief Retrieve the handle to the device compute queue family index.
            ///
            /// @returns The handle to the device compute queue family index.
            uint32_t GetComputeQueueFamilyIndex() const;

            /// @brief Retrieve the handle to the underlying physical device.
            ///
            /// @returns The handle to the underlying physical device.
            VkPhysicalDevice GetPhysicalDevice() const;

            /// @brief Query the physical device memory properties.
            ///
            /// @returns The physical device memory properties info structure.
            VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() const;

            /// @brief Query the physical device properties.
            ///
            /// @returns The physical device properties info structure.
            VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const;

            /// @brief Query the physical device subgroup properties.
            ///
            /// @returns The physical device subgroup properties info structure.
            VkPhysicalDeviceSubgroupProperties GetPhysicalDeviceSubgroupProperties() const;

            /// @brief Get the vulkan instance.
            ///
            /// @returns The vulkan instance.
            VkInstance GetInstance() const;

            /// @brief Get the VMA allocator.
            ///
            /// @return The VMA allocator.
            VmaAllocator GetAllocator() const;

            /// @brief Wait for all in-flight work in the GPU queues to complete.
            void GPUFlush();

            /// @brief Flush the given command buffer, including executing all commands and waiting for completion.
            ///
            /// @param [in] command_buffer The command buffer containing the operations to execute.
            /// @param [in] queue The queue used to submit command buffer operations.
            /// @param [in] pool The pool that the command buffer is part of.
            /// @param [in] free An optional bool which, when set to true, will free the flushed command buffer back to the pool.
            void FlushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, VkCommandPool pool, bool free = true);

            /// @brief Create a buffer with the device using the given buffer configuration.
            ///
            /// @param [in] usage_flags The buffer usage flags.
            /// @param [in] memory_usage The memory usage indicator.
            /// @param [out] buffer The output buffer that was created.
            /// @param [out] allocation The allocation for the buffer.
            /// @param [in] data An pointer to the data to populate the buffer with. If the value is nullptr mapping will be skipped.
            /// @param [in] size The total size of the buffer in bytes.
            void CreateBuffer(VkBufferUsageFlags usage_flags,
                              VmaMemoryUsage     memory_usage,
                              VkBuffer&          buffer,
                              VmaAllocation&     allocation,
                              const void*        data,
                              VkDeviceSize       size);

            /// @brief Transfer CPU buffer to a device local Vulkan buffer. Handles staging buffer and transfer command.
            ///
            /// @param cmd        The command buffer for the transfer command.
            /// @param queue      The queue to submit the transfer command to.
            /// @param buffer     The destination buffer.
            /// @param data       The source CPU buffer.
            /// @param size       The source buffer size.
            void TransferBufferToDevice(VkCommandBuffer cmd, VkQueue queue, VkBuffer buffer, const void* data, VkDeviceSize size);

            /// @brief Create an image with the device using the given image configuration.
            ///
            /// @param [in] image_create_info The image create information.
            /// @param [in] memory_usage The memory usage indicator.
            /// @param [out] image The output image that was created.
            /// @param [out] allocation The allocation for the image.
            void CreateImage(VkImageCreateInfo image_create_info, VmaMemoryUsage memory_usage, VkImage& image, VmaAllocation& allocation);

            /// @brief Write to a buffer allocation.
            ///
            /// @param [out] allocation The allocation for the buffer.
            /// @param [in] data A pointer to the data to populate the buffer with. If the value is nullptr mapping will be skipped.
            /// @param [in] size The total size of the data to write.
            void WriteToBuffer(const VmaAllocation& allocation, const void* data, VkDeviceSize size);

            /// @brief Fill a buffer with all zeroes.
            ///
            /// @param [out] allocation The allocation for the buffer.
            /// @param [in] size The total size of the data to write.
            void ZeroOutBuffer(const VmaAllocation& allocation, VkDeviceSize size);

            /// @brief Read from a buffer allocation.
            ///
            /// @param [out] allocation The allocation for the buffer.
            /// @param [in] data An pointer to the data to copy buffer information.
            /// @param [in] size The total size of the data to read.
            void ReadFromBuffer(const VmaAllocation& allocation, void* data, VkDeviceSize size);

            /// @brief Destroy a buffer in this device.
            ///
            /// @param [inout] buffer The output buffer to be destroyed.
            /// @param [inout] allocation The allocation for the buffer.
            void DestroyBuffer(VkBuffer& buffer, VmaAllocation& allocation);

            /// @brief Destroy a buffer in this device.
            ///
            /// @param [inout] image The output image to be destroyed.
            /// @param [inout] allocation The allocation for the buffer.
            void DestroyImage(VkImage& image, VmaAllocation& allocation);

            /// @brief Get the possible msaa sample settings in descending order.
            ///
            /// @returns A list of possible msaa sample settings.
            std::vector<VkSampleCountFlagBits> GetPossibleMSAASampleSettings();

        private:
            VkInstance                         instance_            = VK_NULL_HANDLE;  ///< The instance handle.
            VkDevice                           device_              = VK_NULL_HANDLE;  ///< The device handle.
            VkPhysicalDevice                   physical_device_     = VK_NULL_HANDLE;  ///< The physical device handle.
            VkPhysicalDeviceMemoryProperties   memory_properties_   = {};              ///< The device memory properties.
            VkPhysicalDeviceProperties         device_properties_   = {};              ///< The device properties.
            VkPhysicalDeviceProperties2        device_properties_2_ = {};              ///< The device properties 2 structure.
            VkPhysicalDeviceSubgroupProperties subgroup_properties_ = {};              ///< The device subgroup properties.
            VkSurfaceKHR                       surface_             = VK_NULL_HANDLE;  ///< The surface used to gather information for the queue creation.
            VmaAllocator                       allocator_           = VK_NULL_HANDLE;  ///< VMA allocator to help with allocation.
            VkQueue                            present_queue_       = VK_NULL_HANDLE;  ///< The device present queue.
            uint32_t                           present_queue_family_index_  = 0;       ///< The device present queue family index.
            VkQueue                            graphics_queue_              = VK_NULL_HANDLE;  ///< The device graphics queue.
            uint32_t                           graphics_queue_family_index_ = 0;               ///< The device graphics queue family index.
            VkQueue                            compute_queue_               = VK_NULL_HANDLE;  ///< The device compute queue.
            uint32_t                           compute_queue_family_index_  = 0;               ///< The device compute queue family index.
            bool                               using_validation_layer       = false;           ///< The flag indicating if the validation layers are active.
            size_t                             buffer_allocation_count_     = 0;               ///< The buffer allocation count.
            size_t                             image_allocation_count_      = 0;               ///< The image allocation count.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_FRAMEWORK_DEVICE_H_
