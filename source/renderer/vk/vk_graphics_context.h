//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for graphics context for the Vulkan API.
//=============================================================================

#ifndef RRA_RENDERER_VK_GRAPHICS_CONTEXT_H_
#define RRA_RENDERER_VK_GRAPHICS_CONTEXT_H_

#include "framework/device.h"
#include "public/renderer_interface.h"
#include <map>

namespace rra
{
    namespace renderer
    {
        /// @brief An instruction to use when drawing a blas.
        struct BlasDrawInstruction
        {
            VkBuffer vertex_buffer = VK_NULL_HANDLE;
            uint32_t vertex_index  = 0;
            uint32_t vertex_count  = 0;
        };

        /// @brief The traversal tree for vulkan.
        struct VkTraversalTree
        {
            VkBuffer      volume_buffer     = VK_NULL_HANDLE;
            VmaAllocation volume_allocation = VK_NULL_HANDLE;
            VkBuffer      vertex_buffer     = VK_NULL_HANDLE;
            VmaAllocation vertex_allocation = VK_NULL_HANDLE;
        };

        /// @brief The Vulkan specific graphics context.
        class VkGraphicsContext
        {
        public:
            /// @brief Initializes the Vulkan context and device.
            ///
            /// @returns True if the graphics context was initialized successfully, or false if a failure occurred.
            bool Initialize(const GraphicsContextSceneInfo& info);

            /// @brief Cleans up the context.
            void Cleanup();

            /// @brief Set the window information for this context.
            ///
            /// @param [in] window_info The window information.
            void SetWindowInfo(WindowInfo window_info);

            /// @brief Get the draw instruction for a blas index.
            ///
            /// @param [in] blas_index The blas index.
            ///
            /// @returns An instruction on how to draw the blas.
            BlasDrawInstruction GetBlasDrawInstruction(uint64_t blas_index);

            /// @brief Get the uploaded traversal tree for a blas index.
            ///
            /// @param [in] blas_index The blas index.
            ///
            /// @returns The loaded memory of the traversal tree.
            VkTraversalTree GetBlasTraversalTree() const;

            /// @brief Get the uploaded traversal tree volume offset for a blas indexes.
            ///
            /// @param [in] blas_index The blas index.
            ///
            /// @returns The index of the root volume.
            uint32_t GetBlasTraversalTreeRootVolumeIndex(uint32_t blas_index) const;

            /// @brief Get the device for this context.
            ///
            /// @returns A device.
            Device& GetDevice();

            /// @brief Check if the context has been initialized successfully and is ready to use.
            ///
            /// @returns True if the graphics context has been initialized successfully.
            bool IsInitialized() const;

            /// @brief Check if the error window has been primed.
            ///
            /// @returns True if the error window has been primed.
            bool IsErrorWindowPrimed();

            /// @brief Check if the context is healthy.
            ///
            /// @returns True if the context is healthy.
            bool IsHealthy();

            /// @brief Get the initialization error message in the event of startup failure.
            ///
            /// @returns The initialization error message.
            std::string GetInitializationErrorMessage();

            /// @brief Set the initialization error message.
            ///
            /// @param [in] message The initialization error message to send.
            void SetInitializationErrorMessage(const std::string& message);

            /// @brief Set the scene info the graphics context will use.
            /// @param scene_info The scene info used for renderering.
            void SetSceneInfo(RendererSceneInfo* scene_info);

        private:
            /// @brief Collects and uploads the traversal trees to the device.
            ///
            /// returns True on successful upload.
            bool CollectAndUploadTraversalTrees(const GraphicsContextSceneInfo& info);

            Device                           device_;                                       ///< The renderer device.
            WindowInfo                       window_info_;                                  ///< The window information.
            std::vector<BlasDrawInstruction> geometry_instructions_;                        ///< Mapping from BLAS to a geometry address.
            VkBuffer                         geometry_buffer_            = VK_NULL_HANDLE;  ///< A geometry buffer.
            VmaAllocation                    geometry_buffer_allocation_ = VK_NULL_HANDLE;  ///< The allocation for the geometry buffer.
            VkTraversalTree                  traversal_tree_;                               ///< The traversal tree.
            std::vector<uint32_t>            traversal_tree_blas_offsets_;                  ///< The offsets of the traversal tree.
            RendererSceneInfo*               scene_info_;                                   ///< Information needed to render the scene.

            /// We load our contents in a seperate thread so we can't show the error window and exit until we join main thread.
            bool error_window_primed_ = false;  /// A flag to track if the error window can be shown if the vulkan crashes after the loading has been complete.
            std::string initialization_error_message_ = "";  /// The error message in case of a crash before rendering starts (aka loading).

            bool initialized_ = false;  ///< A flag used to track if the context has been initialized.
        };

        /// @brief A function to get the global graphics context.
        ///
        /// @return A Vulkan graphics context.
        VkGraphicsContext* GetVkGraphicsContext();

        /// @brief A function to delete the global graphics context.
        void DeleteVkGraphicsContext();

    }  // namespace renderer

}  // namespace rra

#endif  // RRA_RENDERER_VK_GRAPHICS_CONTEXT_H_
