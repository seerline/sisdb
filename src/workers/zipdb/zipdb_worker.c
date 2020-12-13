﻿#include "zipdb.h"
//////////////////////////////////////////////////////////////////
//------------------------s_zipdb_worker -----------------------//
//////////////////////////////////////////////////////////////////
s_zipdb_worker *zipdb_worker_create()
{
	s_zipdb_worker *o = SIS_MALLOC(s_zipdb_worker, o);
	o->cur_sbits = sis_bits_stream_create(NULL, 0);
	o->keys = sis_map_list_create(sis_sdsfree_call);
	o->sdbs = sis_map_list_create(sis_dynamic_db_destroy);
	return o;
}

void zipdb_worker_destroy(s_zipdb_worker *worker)
{
	sis_map_list_destroy(worker->keys);
	sis_map_list_destroy(worker->sdbs);
	sis_bits_stream_destroy(worker->cur_sbits);
	if (worker->zip_obj)
	{
		sis_object_destroy(worker->zip_obj);
	}
	sis_free(worker);
}

void zipdb_worker_unzip_init(s_zipdb_worker *worker, void *cb_source_, cb_sis_struct_decode *cb_read_)
{
	worker->cb_source = cb_source_;
	worker->cb_read = cb_read_;
}

void zipdb_worker_zip_init(s_zipdb_worker *worker, int zipsize, int initsize)
{
	worker->initsize = initsize;
	if (worker->zip_obj) 
	{
		sis_object_destroy(worker->zip_obj);
	}
	worker->zip_size = zipsize + 256;
	s_sis_memory *memory = sis_memory_create_size(worker->zip_size + sizeof(s_zipdb_bits));
	worker->zip_obj = sis_object_create(SIS_OBJECT_MEMORY, memory);
	zipdb_worker_zip_flush(worker, 1);
	// printf("reader->sub_ziper = %p %d\n", worker->zip_obj, worker->zip_size);
}
void zipdb_worker_zip_flush(s_zipdb_worker *worker, int isinit)
{
	if (!worker->zip_obj) 
	{
		return ;
	}
	s_zipdb_bits *zipmem = MAP_ZIPDB_BITS(worker->zip_obj);
	if (isinit)
	{
		zipmem->init = 1;
		sis_bits_struct_flush(worker->cur_sbits);
		worker->cur_size = 0;
	}
	else
	{
		zipmem->init = 0;
	}
	sis_bits_struct_link(worker->cur_sbits, zipmem->data, worker->zip_size);	
	zipmem->size = 0;
	memset(zipmem->data, 0, worker->zip_size);			
}
void zipdb_worker_zip_set(s_zipdb_worker *worker, int kidx, int sidx, char *in_, size_t ilen_)
{
	sis_bits_struct_encode(worker->cur_sbits, kidx, sidx, in_, ilen_);
	// s_zipdb_bits *zipmem = MAP_ZIPDB_BITS(worker->zip_obj);
	// zipmem->size = sis_bits_struct_getsize(worker->cur_sbits);
	// sis_memory_set_size(SIS_OBJ_MEMORY(worker->zip_obj), sizeof(s_zipdb_bits) + zipmem->size);
}
void zipdb_worker_clear(s_zipdb_worker *worker)
{
	LOG(5)("clear unzip reader\n");
	sis_map_list_clear(worker->keys);
	sis_map_list_clear(worker->sdbs);
	sis_bits_stream_clear(worker->cur_sbits);
}

void zipdb_worker_set_keys(s_zipdb_worker *worker, s_sis_sds in_)
{
	LOG(5)("set unzip keys\n");
	// printf("%s\n",in_);
	s_sis_string_list *klist = sis_string_list_create();
	sis_string_list_load(klist, in_, sis_sdslen(in_), ",");
	// 重新设置keys
	int count = sis_string_list_getsize(klist);
	for (int i = 0; i < count; i++)
	{
		s_sis_sds key = sis_sdsnew(sis_string_list_get(klist, i));
		sis_map_list_set(worker->keys, key, key);	
	}
	sis_string_list_destroy(klist);
	sis_bits_struct_set_key(worker->cur_sbits, count);
}
void zipdb_worker_set_sdbs(s_zipdb_worker *worker, s_sis_sds in_)
{
	LOG(5)("set unzip sdbs\n");
	// printf("%s %d\n",in_, sis_sdslen(in_));
	s_sis_json_handle *injson = sis_json_load(in_, sis_sdslen(in_));
	if (!injson)
	{
		return ;
	}
	s_sis_json_node *innode = sis_json_first_node(injson->node);
	while (innode)
	{
		s_sis_dynamic_db *sdb = sis_dynamic_db_create(innode);
		if (sdb)
		{
			sis_map_list_set(worker->sdbs, sdb->name, sdb);
			sis_bits_struct_set_sdb(worker->cur_sbits, sdb);
		}
		innode = sis_json_next_node(innode);
	}
	sis_json_close(injson);
}

int _unzip_nums = 0;
msec_t _unzip_msec = 0;
int _unzip_recs = 0;
msec_t _unzip_usec = 0;

void zipdb_worker_unzip_set(s_zipdb_worker *worker, s_zipdb_bits *in_)
{
	if (in_->init == 1)
	{ 
		LOG(5)("unzip init = %d : %d\n", in_->init, worker->cur_sbits->inited);
		// 这里memset时报过错
		sis_bits_struct_flush(worker->cur_sbits);
		sis_bits_struct_link(worker->cur_sbits, in_->data, in_->size);	
	}
	else
	{
		sis_bits_struct_link(worker->cur_sbits, in_->data, in_->size);
	}
	msec_t _start_usec = sis_time_get_now_usec();
	// 开始解压 并回调
	// int nums = sis_bits_struct_decode(worker->cur_sbits, NULL, NULL);
	int nums = sis_bits_struct_decode(worker->cur_sbits, worker->cb_source, worker->cb_read);
	if (nums == 0)
	{
		LOG(5)("unzip fail.\n");
	}
	if (worker->cb_read)
	{
		// 当前包处理完毕
		worker->cb_read(worker->cb_source, -1, -1, NULL, 0);
	}
	if (_unzip_nums == 0)
	{
		_unzip_msec = sis_time_get_now_msec();
	}
	_unzip_recs+=nums;
	_unzip_nums++;
	_unzip_usec+= (sis_time_get_now_usec() - _start_usec);
	if (_unzip_nums % 1000 == 0)
	{
		printf("unzip cost : %lld. [%d] msec : %lld recs : %d\n", _unzip_usec, _unzip_nums, 
			sis_time_get_now_msec() - _unzip_msec, _unzip_recs);
		_unzip_msec = sis_time_get_now_msec();
		_unzip_usec = 0;
	}
}
