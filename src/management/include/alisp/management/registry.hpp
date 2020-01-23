#pragma once

#include <iostream>

#include <vector>
#include <unordered_map>
#include <array>
#include <memory>
#include <utility>
#include <cstring>

namespace alisp
{

namespace management
{

template<typename T>
struct Resource {
    T res;
    uint32_t id;
};


template<typename T, size_t tag>
class Registry {

  public:

    constexpr static std::uint32_t REG_BITS = 0xFF << 23;
    constexpr static std::uint32_t VALID_BIT = 0x1 << 22;
    constexpr static std::uint32_t TAG_BITS = static_cast<std::uint32_t>(tag) << 23;
    constexpr static std::uint32_t INLINED_BIT = 0x80000000;
    static constexpr size_t INLINED = 10;
    
  private:
    
    std::array<Resource<T>, INLINED> inline_res;
    std::vector<Resource<T>> dyn_res;
    std::vector<uint32_t> free_list;
    
    uint32_t inlined_cnt = 0;

    uint32_t next_id() {
        if (!free_list.empty()) {
            auto i = free_list.back();
            free_list.pop_back();
            return i;
        }
        if (inlined_cnt < INLINED) {
            return (inlined_cnt++ | INLINED_BIT | TAG_BITS | VALID_BIT);
        }
        return (static_cast<std::uint32_t>(dyn_res.size()) & ~INLINED_BIT) | TAG_BITS | VALID_BIT;
    }

    Resource<T>* get_memory(uint32_t t_index) {
        if ((t_index & INLINED_BIT) != 0) {
            return &inline_res[t_index & ~INLINED_BIT & ~REG_BITS & ~VALID_BIT];
        }
        const auto dyn_index = (t_index & ~INLINED_BIT & ~REG_BITS & ~VALID_BIT);
        return dyn_res.data() + dyn_index;
    }

  public:

    Resource<T>* put_resource(T t_res){
        auto id = next_id();
        if ((id & INLINED_BIT) != 0) {
            Resource<T>* mem = get_memory(id);
            new (mem) Resource<T>{t_res, id};
            return mem;
        }

        const auto dyn_id = (id & ~INLINED_BIT & ~REG_BITS);
        dyn_res.insert(dyn_res.begin()+dyn_id, {t_res, id});
        return &dyn_res[dyn_id];
        
    };

    template<typename ... Arg>
    Resource<T>* emplace_resource(Arg ... t_args){
        
        auto id = next_id();
        if ((id & INLINED_BIT) != 0) {
            Resource<T>* mem = get_memory(id);
            new (mem) Resource<T>{T(t_args...), id};
            return mem;
        }
        
        const auto dyn_id = (id & ~INLINED_BIT & ~REG_BITS & ~VALID_BIT);
        dyn_res.insert(dyn_res.begin()+dyn_id, {T(t_args...), id});
        return &dyn_res[dyn_id];    
        
    }

    void destroy_resource(uint32_t t_id){

        auto* mem = get_memory(t_id);
        mem->~Resource<T>();

        mem->id = mem->id & ~VALID_BIT;

        if ((t_id & INLINED_BIT) != 0) {
            --inlined_cnt;
        }
        free_list.push_back(t_id);
    };
    
    Resource<T>* get_resource(uint32_t t_id) {
        return get_memory(t_id);
    }

    bool belong(uint32_t t_id ) {
        
        if (((t_id & REG_BITS) >> 23) == tag) {
            
            
            if (std::find(std::begin(free_list),std::end(free_list), t_id) != std::end(free_list)){
                return false;
            }

            
            return true;
        }
        return false;
    }

    T& operator[](uint32_t t_ind) {
        return get_memory(t_ind)->res;
    }
    
};


}

}