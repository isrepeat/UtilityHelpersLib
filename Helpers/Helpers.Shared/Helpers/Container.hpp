#pragma once
#include "common.h"
#include <deque>
#include <queue>

namespace HELPERS_NS {
    template<class T, class ContainerT>
    class fixed_container : private ContainerT {
    public:
        using Base = ContainerT;
        using Base::Base;

        using Base::size;
        using Base::begin;
        using Base::end;
        using Base::operator[];

        fixed_container(ContainerT&& other)
            : ContainerT(std::move(other))
        {}
    };


    template<class T>
    using fixed_deque = fixed_container<T, std::deque<T>>;


    template<typename T, typename Container = std::deque<T> >
    class iterable_queue : public std::queue<T, Container> {
    public:
        typedef typename Container::iterator iterator;
        typedef typename Container::const_iterator const_iterator;

        iterator begin() { return this->c.begin(); }
        iterator end() { return this->c.end(); }
        const_iterator begin() const { return this->c.begin(); }
        const_iterator end() const { return this->c.end(); }
    };
}