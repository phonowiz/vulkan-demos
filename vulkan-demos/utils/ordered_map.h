//
//  ordered_map.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "EASTL/string_view.h"
#include "EASTL/fixed_vector.h"
#include "EAassert/eaassert.h"
#include <algorithm>

//this container gives us the ability to keep the insertion order of the keys as they are entered
//into the ordered_map object.  It also allows us to lock it's values once we are done populating it.
template< class _key, class _value>
class ordered_map
{
private:

    struct comparitor
    {
        comparitor(_key const& s) : _k(s)
        {}
        
        bool operator () ( eastl::pair< _key, _value > const & p )
        {
            return p.first == _k;
        }
        
        _key _k;
    };
    typedef eastl::fixed_vector< eastl::pair< _key, _value>, 20, true> _vector;
    _vector _vec;
    bool _frozen = false;
    
public:
    template<typename _value_type >
    struct iterator
    {
        _vector& vec_;
        size_t pointer_;
        
        iterator(_vector& vec) : vec_{vec}, pointer_{0} {}
        
        iterator(_vector& vec, size_t size) : vec_{vec}, pointer_{size} {}
        
        bool operator!=(const iterator<_value_type>& other) const
        {
            return !(*this == other);
        }
        
        bool operator==(const iterator<_value_type>& other) const
        {
            return pointer_ == other.pointer_;
        }
        
        iterator& operator++()
        {
            ++pointer_;
            return *this;
        }
        
        eastl::pair< _key, _value>& operator*() const
        {
            return vec_.at(pointer_);
        }
        
        _value& get()
        {
            return vec_.at(pointer_).second;
        }
    };
    
public:
    
    inline void freeze() { _frozen = true;}
    
    inline iterator<_value> begin()
    {
        return iterator<_value>(_vec);
    }
    
    inline iterator<_value> end()
    {
        return iterator<_value>(_vec, _vec.size());
    }
    
    inline iterator<_value> begin() const
    {
        return iterator< _value>(_vec);
    }
    
    inline iterator<_value> end() const
    {
        return iterator< _value> (_vec, _vec.size());
    }
    
    inline size_t size(){ return _vec.size(); }
	inline size_t size() const { return _vec.size(); }
    
    template<const char*, class _val>
    _value & operator[]( const char* i)
    {
        eastl::string_view key(i);
        
        for( auto& element : _vec)
        {
            eastl::string_view  first (element.first);
            if( key.compare(first) == 0)
            {
                return element.second;
            }
        }
        EA_ASSERT_FORMATTED(!_frozen, ("you are looking for key %s that doesn't exist, did you forget to initialize it?", i));
        _value val;
        eastl::pair<_key, _value> p =  std::make_pair(i, val);
        _vec.push_back(p);
        return _vec.back().second;
    }
    inline iterator<_value> find(_key i)
    {
        size_t index =0;
        for( auto& element : _vec)
        {
            if( element.first == i)
            {
                return iterator<_value>(_vec, index);
            }
            ++index;
        }
        
        return end();
    }
    inline _value& operator [](_key i)
    {
        iterator<_value > p = find(i);
        
        if( p == end())
        {
            EA_ASSERT_MSG(!_frozen,"you are looking for key that doesn't exist, did you forget to initialize this key?");
            _value val;
            eastl::pair<_key, _value> p =  eastl::make_pair(i, val);
            _vec.push_back(p);
            return _vec.back().second;
        }
        return (p).get();
    }
    
    ordered_map<_key, _value>& operator=( const ordered_map<_key, _value>& right)
    {
        for( auto& element : right._vec)
        {
            _vec.push_back(element);
        }
        _frozen = false;
        return *this;
    }
    
    void clear()
    {
        _vec.clear();
    }
    
};

