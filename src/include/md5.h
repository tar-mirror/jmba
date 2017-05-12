#ifndef MD5_H
#define MD5_H

#ifdef __alpha
typedef unsigned int uint32;
#else
typedef unsigned long uint32;
#endif

struct MD5Context {
        uint32 buf[4];
        uint32 bits[2];
        unsigned char in[64];
};

typedef struct MD5Context MD5_CTX;

extern void MD5Init(MD5_CTX *);
extern void MD5Update(MD5_CTX *, unsigned char *, uint32);
extern void MD5Final(unsigned char *, MD5_CTX *);
extern void MD5Transform(uint32 *, uint32 *);

#endif /* !MD5_H */
