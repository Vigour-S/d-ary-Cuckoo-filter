/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _BITS_H_
#define _BITS_H_
#define MAX 15

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <string.h>

namespace d_ary_cuckoofilter {
    
    inline uint64_t upperpower2(uint64_t x) {
        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x |= x >> 32;
        x++;
        return x;
    }
    
    inline uint64_t upperpower3(uint64_t x)
    {
        return (uint64_t)pow(3, ceil(log(x)/log(3)));
    }
    
    inline uint64_t upperpower4(uint64_t x)
    {
        return (uint64_t)pow(4, ceil(log(x)/log(4)));
    }
    
    inline uint64_t upperpower5(uint64_t x)
    {
        return (uint64_t)pow(5, ceil(log(x)/log(5)));
    }
    
    static const char* tbl[] =
    {
        "0000",
        "0001",
        "0010",
        "0011",
        "0100",
        "0101",
        "0110",
        "0111",
        "1000",
        "1001",
        "1010",
        "1011",
        "1100",
        "1101",
        "1110",
        "1111"
    };
    
    inline void printFingerprint(unsigned char uc, uint32_t t) {
        printf("%s%s", tbl[uc >> 4], tbl[uc& 0xF]);
        std::cout << "fingerprint:" << t << std::endl;
    }
    
    inline void base10toX(size_t a[], size_t t, size_t x) {
        size_t m = t;
        size_t i = 0;
        while(t)
        {
            t/=x;
            a[i++]=m-x*t;
            m=t;
        }
    }
    
    inline size_t baseXto10(size_t a[], size_t x) {
        size_t base10 = 0;
        for( size_t i = 0; i < MAX; i++ ) {
            base10 = a[i]*pow(x, i) + base10;
        }
        return base10;
    }
    
    inline size_t xor_(size_t fingerprint, size_t index, size_t base) {
        size_t a[MAX], b[MAX], result[MAX];
        memset(a, 0, sizeof(a));
        memset(b, 0, sizeof(b));
        memset(result, 0, sizeof(result));
        base10toX(a, fingerprint, base);
        base10toX(b, index, base);
        for (size_t i = 0; i < MAX; i++) {
            result[i] = (a[i] + b[i]) % base;
        }
        return baseXto10(result, base);
    }
    
    inline size_t markbits(size_t t) {
        size_t i = 0;
        while(t)
        {
            t/=2;
            i++;
        }
        return i;
    }
}
#endif //_BITS_H
