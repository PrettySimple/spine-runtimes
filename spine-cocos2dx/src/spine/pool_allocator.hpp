//
//  pool_allocator.hpp
//  Spine
//
//  Created by Mathieu Garaud on 06/05/2018.
//  Copyright © 2018 Pretty Simple. All rights reserved.
//

#ifndef SPINE_POOL_ALLOCATOR_HPP
#define SPINE_POOL_ALLOCATOR_HPP

#include "page.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

namespace spine
{
    template <typename T>
    class pool_allocator final
    {
    public:
        using pointer = T*;
        using value_type = page;

    private:
        std::vector<std::unique_ptr<value_type>> _pages;
        std::vector<std::pair<pointer, std::size_t>> _pointers;

    public:
        pointer allocate(std::size_t n)
        {
            auto ret = _allocate(n);
            if constexpr(std::is_default_constructible<T>::value)
            {
                for (std::size_t i = 0; i < n; ++i)
                {
                    new (ret+i) T();
                }
            }
            return ret;
        }

        void deallocate(pointer p, std::size_t n)
        {
        }

        void deallocate_all()
        {
            if constexpr(std::is_destructible<T>::value)
            {
                for (auto const& [ptr, size] : _pointers)
                {
                    for (std::size_t i = 0; i < size; ++i)
                    {
                        (ptr + i)->~T();
                    }
                }
                _pointers.clear();
            }

            for (auto const& page : _pages)
            {
                page->deallocate_all();
            }
        }

    private:
        pointer _allocate(std::size_t n)
        {
            auto const byte_size = n * sizeof(T);
            static constexpr auto const align = alignof(T);
            for(std::size_t i = 0, max = _pages.size(); i < max;)
            {
                if (pointer ret = reinterpret_cast<pointer>(_pages[i]->allocate(byte_size, align)); ret != nullptr)
                {
                    if constexpr(std::is_destructible<T>::value)
                    {
                        _pointers.emplace_back(ret, n);
                    }
                    return ret;
                }
                else if (max > 1 && (i + 1) < max && _pages[i]->usage() > 0.90f && _pages[i + 1]->usage() < 0.90f)
                {
                    std::rotate(_pages.begin() + i, _pages.begin() + i + 1, _pages.end());
                }
                else
                {
                    ++i;
                }
            }

            _pages.emplace_back(std::make_unique<value_type>(byte_size));
            pointer ret = reinterpret_cast<pointer>(_pages.back()->allocate(byte_size, align));
            if constexpr(std::is_destructible<T>::value)
            {
                _pointers.emplace_back(ret, n);
            }
            return ret;
        }

    };
}

#endif // SPINE_POOL_ALLOCATOR_HPP