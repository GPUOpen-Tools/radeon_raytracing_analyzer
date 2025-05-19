//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Renderer Adapter interface. This component can
///         be used by UI code to query and alter various renderer states.
//=============================================================================

#ifndef RRA_RENDERER_RENDERER_ADAPTER_H_
#define RRA_RENDERER_RENDERER_ADAPTER_H_

#include <cstdint>
#include <map>

namespace rra
{
    namespace renderer
    {
        enum class RendererAdapterType : uint8_t
        {
            kRendererAdapterTypeView,
            kRendererAdapterTypeRenderState,
        };

        class RendererAdapter
        {
        public:
            /// @brief Constructor.
            RendererAdapter() = default;

            /// @brief Destructor.
            virtual ~RendererAdapter() = default;
        };

        /// @brief A map used to associate a renderer adapter type with an instance.
        typedef std::map<RendererAdapterType, RendererAdapter*> RendererAdapterMap;

        /// @brief Template method to get an adapter by type.
        ///
        /// @param [in] adapters The list of adapters.
        /// @param [in] type The type of adapter to get.
        ///
        /// @return The derived class if found, or nullptr.
        template <class AdapterType>
        AdapterType GetAdapter(const RendererAdapterMap& adapters, RendererAdapterType type)
        {
            AdapterType result = nullptr;

            auto render_state_adapter_iter = adapters.find(type);
            if (render_state_adapter_iter != adapters.end())
            {
                result = dynamic_cast<AdapterType>(render_state_adapter_iter->second);
            }

            return result;
        }

    }  // namespace renderer

}  // namespace rra

#endif  // RRA_RENDERER_RENDERER_ADAPTER_H_

