/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          Copyright (c) 2002 Christopher Clark                           |
|          Licensed under the Apache License, Version 2.0            |
\===================================================================*/

/**
 * @file hashtable_private.h
 * @brief Hashtable implementation
 *
 * @date 2002
 * @author Christopher Clark <firstname.lastname@cl.cam.ac.uk>
 */

#ifndef __HASHTABLE_PRIVATE_CWC22_H__
#define __HASHTABLE_PRIVATE_CWC22_H__

#include "hashtable.h"

/*****************************************************************************/
struct entry
{
    String key;
    Index value;
    uint32_t hash;
    struct entry *next;
};

struct hashtable {
    unsigned int tablelength;
    struct entry **table;
    unsigned int entrycount;
    unsigned int loadlimit;
    unsigned int primeindex;
    uint32_t (*hashfn) (String key);
    bool (*eqfn) (const String str1, const String str2);
};

/*****************************************************************************/

// uint32_t hash(struct hashtable *h, void *k, unsigned int len);

/*****************************************************************************/
/* indexFor */
/* inline not supported by all platforms
   static inline unsigned int
indexFor(unsigned int tablelength, unsigned int hashvalue) {
    return (hashvalue % tablelength);
};
*/
#define indexFor(tablelength,hashvalue)		((unsigned int) (hashvalue % (uint32_t) tablelength))

/* Only works if tablelength == 2^N */
/*static inline unsigned int
indexFor(unsigned int tablelength, unsigned int hashvalue)
{
    return (hashvalue & (tablelength - 1u));
}
*/

/*****************************************************************************/
#define freekey(X) EXIP_MFREE(X)
/*define freekey(X) ; */


/*****************************************************************************/

#endif /* __HASHTABLE_PRIVATE_CWC22_H__*/
