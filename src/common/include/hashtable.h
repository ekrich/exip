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

#ifndef __HASHTABLE_CWC22_H__
#define __HASHTABLE_CWC22_H__

#include "exipConfig.h"
#include "procTypes.h"

/**
 * DJB Hash Function
 * An algorithm produced by Professor Daniel J. Bernstein and shown first to the world
 * on the usenet newsgroup comp.lang.c. It is one of the most efficient hash functions ever published.
 *
 * djb2
 * this algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c.
 * another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i];
 * the magic of number 33 (why it works better than many other constants, prime or not) has never
 * been adequately explained.
 **/
uint32_t djbHash(String str);

struct hashtable;

/* Example of use:
 *
 *      struct hashtable  *h;
 *      struct some_key   *k;
 *      struct some_value *v;
 *
 *      static unsigned int         hash_from_key_fn( void *k );
 *      static int                  keys_equal_fn ( void *key1, void *key2 );
 *
 *      h = create_hashtable(16, hash_from_key_fn, keys_equal_fn);
 *      k = (struct some_key *)     malloc(sizeof(struct some_key));
 *      v = (struct some_value *)   malloc(sizeof(struct some_value));
 *
 *      (initialise k and v to suitable values)
 *
 *      if (! hashtable_insert(h,k,v) )
 *      {     exit(-1);               }
 *
 *      if (NULL == (found = hashtable_search(h,k) ))
 *      {    printf("not found!");                  }
 *
 *      if (NULL == (found = hashtable_remove(h,k) ))
 *      {    printf("Not found\n");                 }
 *
 */

/* Macros may be used to define type-safe(r) hashtable access functions, with
 * methods specialized to take known key and value types as parameters.
 *
 * Example:
 *
 * Insert this at the start of your file:
 *
 * DEFINE_HASHTABLE_INSERT(insert_some, struct some_key, struct some_value);
 * DEFINE_HASHTABLE_SEARCH(search_some, struct some_key, struct some_value);
 * DEFINE_HASHTABLE_REMOVE(remove_some, struct some_key, struct some_value);
 *
 * This defines the functions 'insert_some', 'search_some' and 'remove_some'.
 * These operate just like hashtable_insert etc., with the same parameters,
 * but their function signatures have 'struct some_key *' rather than
 * 'void *', and hence can generate compile time errors if your program is
 * supplying incorrect data as a key (and similarly for value).
 *
 * Note that the hash and key equality functions passed to create_hashtable
 * still take 'void *' parameters instead of 'some key *'. This shouldn't be
 * a difficult issue as they're only defined and passed once, and the other
 * functions will ensure that only valid keys are supplied to them.
 *
 * The cost for this checking is increased code size and runtime overhead
 * - if performance is important, it may be worth switching back to the
 * unsafe methods once your program has been debugged with the safe methods.
 * This just requires switching to some simple alternative defines - eg:
 * #define insert_some hashtable_insert
 *
 */

/*****************************************************************************
 * create_hashtable

 * @name                    create_hashtable
 * @param   minsize         minimum initial size of hashtable
 * @param   hashfunction    function for hashing keys
 * @param   key_eq_fn       function for determining key equality
 * @return                  newly created hashtable or NULL on failure
 */

struct hashtable *
create_hashtable(unsigned int minsize,
				 uint32_t (*hashfn) (String key),
				 bool (*eqfn) (const String str1, const String str2));

/*****************************************************************************
 * hashtable_insert

 * @name        hashtable_insert
 * @param   h   the hashtable to insert into
 * @param   k   the key - hashtable claims ownership and will free on removal
 * @param   v   the value - does not claim ownership
 * @return      EXIP_OK for successful insertion
 *
 * This function will cause the table to expand if the insertion would take
 * the ratio of entries to table size over the maximum load factor.
 *
 * This function does not check for repeated insertions with a duplicate key.
 * The value returned when using a duplicate key is undefined -- when
 * the hashtable changes size, the order of retrieval of duplicate key
 * entries is reversed.
 * If in doubt, remove before insert.
 */

errorCode hashtable_insert(struct hashtable *h, String key, Index value);

/*
//#define DEFINE_HASHTABLE_INSERT(fnname, keytype, valuetype) \
//int fnname (struct hashtable *h, keytype *k, unsigned int len, valuetype *v) \
//{ \
//    return hashtable_insert(h,k,len,v); \
//}
*/

/*****************************************************************************
 * hashtable_search

 * @name        hashtable_search
 * @param   h   the hashtable to search
 * @param   k   the key to search for  - does not claim ownership
 * @return      the value associated with the key, or INDEX_MAX if none found
 */

Index hashtable_search(struct hashtable *h, String key);

/*
//#define DEFINE_HASHTABLE_SEARCH(fnname, keytype, valuetype) \
//valuetype * fnname (struct hashtable *h, keytype *k, unsigned int len) \
//{ \
//    return (valuetype *) (hashtable_search(h,k,len)); \
//}
*/

/*****************************************************************************
 * hashtable_remove

 * @name        hashtable_remove
 * @param   h   the hashtable to remove the item from
 * @param   k   the key to search for  - does not claim ownership
 * @return      the value associated with the key, or INDEX_MAX if none found
 */

Index hashtable_remove(struct hashtable *h, String key);

/*
//#define DEFINE_HASHTABLE_REMOVE(fnname, keytype, valuetype) \
//valuetype * fnname (struct hashtable *h, keytype *k, unsigned int len) \
//{ \
//    return (valuetype *) (hashtable_remove(h,k,len)); \
//}
*/

/*****************************************************************************
 * hashtable_count

 * @name        hashtable_count
 * @param   h   the hashtable
 * @return      the number of items stored in the hashtable
 */
unsigned int hashtable_count(struct hashtable *h);


/*****************************************************************************
 * hashtable_destroy

 * @name        hashtable_destroy
 * @param   h   the hashtable
 * @param       free_values     whether to call 'free' on the remaining values
 */

void hashtable_destroy(struct hashtable *h);

#endif /* __HASHTABLE_CWC22_H__ */
