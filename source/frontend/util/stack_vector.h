//=============================================================================
// Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of a vector-like class that is entirely stack allocated to avoid allocation/deletion bottlenecks.
//=============================================================================

#ifndef RRA_UTIL_STACK_VECTOR_H_
#define RRA_UTIL_STACK_VECTOR_H_

#include <array>
#include <vector>
#include <assert.h>
#include <cstdint>

namespace rra
{
    template <typename T, size_t MaxSize>
    class StackVector
    {
    public:
        StackVector()
        {
        }

        ~StackVector()
        {
            if (fallback_vector_)
            {
                delete fallback_vector_;
            }
        }

        void PushBack(const T& t)
        {
            if (fallback_vector_)
            {
                fallback_vector_->push_back(t);
            }
            else if (size_ >= buffer_.size())
            {
                fallback_vector_ = new std::vector<T>{};
                fallback_vector_->resize(size_);
                std::memcpy(fallback_vector_->data(), buffer_.data(), size_ * sizeof(T));
                fallback_vector_->push_back(t);
            }
            else
            {
                buffer_[size_++] = t;
            }
        }

        void PushBack(T&& t)
        {
            if (fallback_vector_)
            {
                fallback_vector_->push_back(std::move(t));
            }
            else if (size_ >= buffer_.size())
            {
                fallback_vector_ = new std::vector<T>{};
                fallback_vector_->resize(size_);
                std::memcpy(fallback_vector_->data(), buffer_.data(), size_ * sizeof(T));
                fallback_vector_->push_back(std::move(t));
            }
            else
            {
                buffer_[size_++] = std::move(t);
            }
        }

        size_t Size() const
        {
            if (fallback_vector_)
            {
                return fallback_vector_->size();
            }
            else
            {
                return size_;
            }
        }

        void Clear()
        {
            if (fallback_vector_)
            {
                fallback_vector_->clear();
            }
            else
            {
                size_ = 0;
            }
        }

        bool Empty()
        {
            if (fallback_vector_)
            {
                return fallback_vector_->empty();
            }
            else
            {
                return size_ == 0;
            }
        }

        void Resize(size_t new_size)
        {
            if (fallback_vector_)
            {
                fallback_vector_->resize(new_size);
            }
            else if (new_size > MaxSize)
            {
                fallback_vector_ = new std::vector<T>{};
                fallback_vector_->resize(new_size);
                std::memcpy(fallback_vector_->data(), buffer_.data(), size_ * sizeof(T));
            }
            else
            {
                size_ = new_size;
            }
        }

        T* Data()
        {
            if (fallback_vector_)
            {
                return fallback_vector_->data();
            }
            else
            {
                return &buffer_[0];
            }
        }

        const T* Data() const
        {
            if (fallback_vector_)
            {
                return fallback_vector_->data();
            }
            else
            {
                return &buffer_[0];
            }
        }

        T& operator[](size_t idx)
        {
            if (fallback_vector_)
            {
                return (*fallback_vector_)[idx];
            }
            else
            {
                assert(idx < size_);
                return buffer_[idx];
            }
        }

        T* begin()
        {
            if (fallback_vector_)
            {
                return &(*fallback_vector_)[0];
            }
            else
            {
                return &buffer_[0];
            }
        }

        const T* begin() const
        {
            if (fallback_vector_)
            {
                return &(*fallback_vector_)[0];
            }
            else
            {
                return &buffer_[0];
            }
        }

        T* end()
        {
            if (fallback_vector_)
            {
                return &(*fallback_vector_)[0] + fallback_vector_->size();
            }
            else
            {
                // Don't use [] notation since this does a bounds check.
                return &buffer_[0] + size_;
            }
        }

        const T* end() const
        {
            if (fallback_vector_)
            {
                return &(*fallback_vector_)[0] + fallback_vector_->size();
            }
            else
            {
                return &buffer_[0] + size_;
            }
        }

    private:
        std::array<T, MaxSize> buffer_{};  ///< The underlying stack allocated array.
        size_t                 size_{};    ///< The size of the dynamic array.

        std::vector<T>* fallback_vector_{nullptr};  ///< If size_ exceeds MaxSize we fall back to vector to prevent crash at the cost of performance.
    };

}  // namespace rra

#endif  // RRA_UTIL_STACK_VECTOR_H_
