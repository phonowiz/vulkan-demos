//
//  ordered_map.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <string>
#include <vector>
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
        
        bool operator () ( std::pair< _key, _value > const & p )
        {
            return p.first == _k;
        }
        
        _key _k;
    };
    typedef std::vector< std::pair< _key, _value>> _vector;
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
        
        std::pair< _key, _value>& operator*() const
        {
            return vec_.at(pointer_);
        }
    };
    
public:
    
    void freeze() { _frozen = true;}
    
    iterator<_value> begin()
    {
        return iterator<_value>(_vec);
    }
    
    iterator<_value> end()
    {
        return iterator<_value>(_vec, _vec.size());
    }
    
    iterator< _value> begin() const
    {
        return iterator< _value>(_vec);
    }
    
    iterator< _value> end() const
    {
        return iterator< _value> (_vec, _vec.size());
    }
    
    inline size_t size(){ return _vec.size(); }
	inline size_t size() const { return _vec.size(); }
    
    template<const char*, class _val>
    _value & operator[]( const char* i)
    {
        std::string_view key(i);
        
        for( auto& element : _vec)
        {
            std::string_view  first (element.first);
            if( key.compare(first) == 0)
            {
                return element.second;
            }
        }
        assert(!_frozen && "you are looking for key that doesn't exist lock has happened");
        _value val;
        std::pair<_key, _value> p =  std::make_pair(i, val);
        _vec.push_back(p);
        return _vec.back().second;
    }
    _value & operator [](_key i)
    {
        for( auto& element : _vec)
        {
            if( element.first == i)
            {
                return element.second;
            }
        }
        assert(!_frozen && "you are looking for key that doesn't exist lock has happened");
        _value val;
        std::pair<_key, _value> p =  std::make_pair(i, val);
        _vec.push_back(p);
        return _vec.back().second;
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
    
};

