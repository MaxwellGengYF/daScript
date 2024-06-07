#pragma once

// #include "daScript/misc/wyhash.h"
#include <xxhash.h>

/*
    this is where we configure low level hash implementation
*/

#define HASH_EMPTY64     0
#define HASH_KILLED64    1
#define DAS_WYHASH_SEED  0x1234567890abcdeful

#ifndef DAS_SAFE_HASH
#define DAS_SAFE_HASH    0
#endif

namespace das {

    static __forceinline uint64_t hash_block64 ( const uint8_t * block, size_t size ) {
        return hash64(block, size, hash64_default_seed);
    }

    static NO_ASAN_INLINE uint64_t hash_blockz64 ( const uint8_t * block ) {
        auto FNV_offset_basis = 14695981039346656037ul;
        auto FNV_prime = 1099511628211ul;
        if ( !block ) return FNV_offset_basis;
        auto h = FNV_offset_basis;
#if DAS_SAFE_HASH
        while ( *block ) {
            h ^= *block++;
            h *= FNV_prime;
        }
#else
        while ( true ) {
            uint64_t v = *(uint16_t *)block;
            if ( (v & 0xff)==0 ) break;
            h ^= v;
            h *= FNV_prime;
            if ( v < 0x100 ) break;
            block += 2;
        }
#endif
        return h <= HASH_KILLED64 ? 1099511628211ul : h;
    }

    static __forceinline uint64_t hash_uint32 ( uint32_t value ) {  // this is simplified, and not the same as wyhash(&value,4)
        return hash64(&value, sizeof(value), hash64_default_seed);
    }

    static __forceinline uint64_t hash_uint64 ( uint64_t value ) { // this is simplified, and not the same as wyhash(&value,4)
        return hash64(&value, sizeof(value), hash64_default_seed);
    }

    class HashBuilder {
        uint64_t seed = hash64_default_seed;
    public:
        uint64_t getHash() {
            return seed <= HASH_KILLED64 ? 1099511628211ul : seed;
        }
        __forceinline void updateString ( const char * str ) {
            if (!str) str = "";
            seed = hash64((const uint8_t *)str, strlen(str), seed);
        }
        __forceinline void updateString ( const char * str, size_t len) {
            if (!str) str = "";
            seed = hash64((const uint8_t *)str, len, seed);
        }
        template <typename TT>
        __forceinline void update ( TT & data ) {
            seed = hash64((const uint8_t *)&data, sizeof(data), seed);
        }
    };
}
