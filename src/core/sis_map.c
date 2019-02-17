﻿
#include "sis_map.h"
#include "sis_list.h"

uint64_t _sis_dict_sds_hash(const void *key)
{
	return sis_dict_hash_func((unsigned char *)key, sis_sdslen((char *)key));
}

uint64_t _sis_dict_sdscase_hash(const void *key)
{
	return sis_dict_casehash_func((unsigned char *)key, sis_sdslen((char *)key));
}

int _sis_dict_sdscase_compare(void *privdata, const void *key1, const void *key2)
{
	SIS_NOTUSED(privdata);
	return sis_strcasecmp((const char *)key1, (const char *)key2) == 0;
}

void _sis_dict_buffer_free(void *privdata, void *val)
{
	SIS_NOTUSED(privdata);
	sis_free(val);
}
void *_sis_dict_sds_dup(void *privdata, const void *val)
{
	SIS_NOTUSED(privdata);
	return sis_sdsnew(val);
}
void _sis_dict_sds_free(void *privdata, void *val)
{
	SIS_NOTUSED(privdata);
	sis_sdsfree((s_sis_sds)val);
}
void _sis_dict_list_free(void *privdata, void *val)
{
	SIS_NOTUSED(privdata);
	sis_struct_list_destroy((s_sis_struct_list *)val);
}

s_sis_dict_type _sis_dict_type_buffer_s = {
	_sis_dict_sdscase_hash,	   /* hash function */
	NULL,				   /* key dup */
	NULL,				   /* val dup */
	_sis_dict_sdscase_compare, /* key compare */
	_sis_dict_sds_free,	 /* key destructor */
	_sis_dict_buffer_free   /* val destructor */
};
s_sis_dict_type _sis_dict_type_owner_free_val_s = {
	_sis_dict_sdscase_hash,	   /* hash function */
	NULL,				   /* key dup */
	NULL,				   /* val dup */
	_sis_dict_sdscase_compare, /* key compare */
	_sis_dict_sds_free,	   /* key destructor */
	NULL				   /* val destructor */
};
s_sis_dict_type _sis_dict_type_sds_s = {
	_sis_dict_sdscase_hash,	   /* hash function */
	_sis_dict_sds_dup,		   /* key dup */
	_sis_dict_sds_dup,		   /* val dup */
	_sis_dict_sdscase_compare, /* key compare */
	_sis_dict_sds_free,	 /* key destructor */
	_sis_dict_sds_free	  /* val destructor */
};
s_sis_dict_type _sis_dict_list_sds_s = {
	_sis_dict_sdscase_hash,	   /* hash function */
	NULL,				   /* key dup */
	NULL,				   /* val dup */
	_sis_dict_sdscase_compare, /* key compare */
	_sis_dict_sds_free,	 /* key destructor */
	_sis_dict_list_free	 /* val destructor */
};

//////////////////////////////////////////
//  s_sis_map_buffer 基础定义
///////////////////////////////////////////////

s_sis_map_buffer *sis_map_buffer_create()
{ //明确知道val的长度
	s_sis_map_buffer *map = sis_dict_create(&_sis_dict_type_buffer_s, NULL);
	return map;
};
void sis_map_buffer_destroy(s_sis_map_buffer *map_)
{
	sis_dict_destroy(map_);
};
void sis_map_buffer_clear(s_sis_map_buffer *map_)
{
	sis_dict_empty(map_, NULL);
};
void *sis_map_buffer_get(s_sis_map_buffer *map_, const char *key_)
{
	if (!map_)
	{
		return NULL;
	}
	s_sis_dict_entry *he;
	s_sis_sds key = sis_sdsnew(key_);
	he = sis_dict_find(map_, key);
	sis_sdsfree(key);
	if (!he)
	{
		return NULL;
	}
	return sis_dict_getval(he);
};
int sis_map_buffer_set(s_sis_map_buffer *map_, const char *key_, void *val_)
{
	s_sis_dict_entry *he;
	s_sis_sds key = sis_sdsnew(key_);
	he = sis_dict_find(map_, key);
	if (!he)
	{
		sis_dict_add(map_, key, val_);
		return 0;
	}
	sis_dict_replace(map_, key, val_);
	sis_sdsfree(key);	
	return 0;
}

//////////////////////////////////////////
//  s_sis_map_int 基础定义
//////////////////////////////////////////
s_sis_map_pointer *sis_map_custom_create(s_sis_dict_type *type_)
{
	s_sis_map_pointer *map = sis_dict_create(type_, NULL);
	return map;
}

s_sis_map_pointer *sis_map_pointer_create()
{
	s_sis_map_pointer *map = sis_dict_create(&_sis_dict_type_owner_free_val_s, NULL);
	return map;
};

//////////////////////////////////////////
//  s_sis_map_int 基础定义
//////////////////////////////////////////
// s_sis_map_int *sis_map_int_create()
// {
// 	s_sis_map_int *map = sis_dict_create(&_sis_dict_type_owner_free_val_s, NULL);
// 	return map;
// };
// uint64_t sis_map_int_get(s_sis_map_int *map_, const char *key_)
// {
// 	s_sis_dict_entry *he;
// 	s_sis_sds key = sis_sdsnew(key_);
// 	he = sis_dict_find(map_, key);
// 	sis_sdsfree(key);	
// 	if (!he)
// 	{
// 		return 0;
// 	}
// 	return sis_dict_get_uint(he);
// };
// int sis_map_int_set(s_sis_map_int *map_, const char *key_, uint64_t value_)
// {
// 	s_sis_dict_entry *he;
// 	s_sis_sds key = sis_sdsnew(key_);
// 	he = sis_dict_find(map_, key);
// 	if (!he)
// 	{
// 		sis_dict_add(map_, key, NULL);

// 		return 0;
// 	}
// 	sis_dict_set_uint(he, value_);
// 	sis_sdsfree(key);	
// 	return 0;
// }
//////////////////////////////////////////
//  s_sis_map_sds 基础定义
//////////////////////////////////////////
s_sis_map_sds *sis_map_sds_create()
{
	s_sis_map_sds *map = sis_dict_create(&_sis_dict_type_sds_s, NULL);
	return map;
};
int sis_map_sds_set(s_sis_map_sds *map_, const char *key_, char *val_)
{
	s_sis_dict_entry *he;
	s_sis_sds key = sis_sdsnew(key_);
	he = sis_dict_find(map_, key);
	if (!he)
	{
		sis_dict_add(map_, key, val_);
		return 0;
	}
	sis_dict_replace(map_, key, val_);
	sis_sdsfree(key);	
	return 0;
}
#if 1

int main()
{
	safe_memory_start();
	s_sis_map_buffer *map = sis_map_buffer_create();
	
	char *str = sis_malloc(100);
	sis_map_buffer_set(map, "key1", str);
	printf("in %p %p\n",map, str);
	char *sss = sis_map_buffer_get(map, "key1");
	printf("out %p %p\n",map, sss);

	str = sis_malloc(100);
	sis_map_buffer_set(map, "key1", str);
	printf("in %p %p\n",map, str);
	sss = sis_map_buffer_get(map, "key1");
	printf("out %p %p\n",map, sss);

	str = sis_malloc(100);
	sis_map_buffer_set(map, "key1", str);
	printf("in %p %p\n",map, str);
	sss = sis_map_buffer_get(map, "key1");
	printf("out %p %p\n",map, sss);

	sis_map_buffer_destroy(map);
	safe_memory_stop();
}
#endif
