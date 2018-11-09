﻿
#include "sisdb_table.h"
#include "sisdb_collect.h"
#include "sisdb_map.h"
#include "sisdb.h"

 // 目前只支持3个必须的值
void _sisdb_table_load_default(s_sis_db *db_, s_sis_json_node *default_)
{
	s_sisdb_sysinfo *info = sis_struct_list_first(db_->info);
	if (!info) 
	{
		info = (s_sisdb_sysinfo *)sis_malloc(sizeof(s_sisdb_sysinfo));
		memset(info, 0 ,sizeof(s_sisdb_sysinfo));
		info->trade_time = sis_struct_list_create(sizeof(s_sis_time_pair), NULL, 0);
		sis_pointer_list_push(db_->info, info);
	}
	if (sis_json_cmp_child_node(default_, "dot")) 
	{
		info->dot = sis_json_get_int(default_, "dot", 2);
	}
	if (sis_json_cmp_child_node(default_, "coinunit")) 
	{
		info->prc_unit = sis_json_get_int(default_, "coinunit", 2);
	}
	if (sis_json_cmp_child_node(default_, "volunit")) 
	{
		info->vol_unit = sis_json_get_int(default_, "volunit", 2);
	}
	s_sis_json_node *wtnode = sis_json_cmp_child_node(default_, "work-time");
	if (wtnode) 
	{
        info->work_time.first = sis_json_get_int(wtnode, "0", 900);
        info->work_time.second = sis_json_get_int(wtnode, "1", 1530);
	}
	s_sis_json_node *ttnode = sis_json_cmp_child_node(default_, "trade-time");
	if (ttnode) 
	{
		int index = 0;
	   	s_sis_time_pair pair;
       	s_sis_json_node *next = sis_json_first_node(ttnode);
        while (next)
        {
            pair.first = sis_json_get_int(next, "0", 930);
            pair.second = sis_json_get_int(next, "1", 1130);
			if (index < info->trade_time->len) {
				sis_struct_list_push(info->trade_time, &pair);
			} else {
	            sis_struct_list_update(info->trade_time, index, &pair);
			}
			index++;
            // printf("trade time [%d, %d]\n",pair.begin,pair.end);
            next = next->next;
        } 
	}	
}

//com_为一个json格式字段定义
s_sisdb_table *sisdb_table_create(s_sis_db *db_, const char *name_, s_sis_json_node *com_)
{
	s_sisdb_table *tb = sisdb_get_table(db_, name_);
	if (tb)
	{
		sisdb_table_destroy(tb);
	}
	// 先加载默认配置
	tb = sis_malloc(sizeof(s_sisdb_table));
	memset(tb, 0, sizeof(s_sisdb_table));

	tb->control.type = SIS_TABLE_TYPE_STS; // 默认保存的目前都是struct，
	tb->control.scale = SIS_TIME_SCALE_SECOND;
	tb->control.limits = sis_json_get_int(com_, "limit", 0);
	tb->control.isinit = sis_json_get_int(com_, "isinit", 0);
	tb->control.issubs = 0;
	tb->control.iszip = 0;

	tb->version = (uint32)sis_time_get_now();
	tb->name = sis_sdsnew(name_);
	tb->father = db_;
	tb->append_method = SIS_ADD_METHOD_ALWAYS;

	sis_dict_add(db_->dbs, sis_sdsnew(name_), tb);

	// 加载实际配置
	s_sis_map_define *mm = NULL;
	const char *strval = NULL;

	s_sis_json_node *node = sis_json_cmp_child_node(com_, "default");
	if (node) 
	{
		_sisdb_table_load_default(db_, node);
	}
	
	strval = sis_json_get_str(com_, "scale");
	mm = sisdb_find_map_define(db_->map, strval, SIS_MAP_DEFINE_TIME_SCALE);
	if (mm)
	{
		tb->control.scale = mm->uid;
	}

	strval = sis_json_get_str(com_, "append-method");
	int nums = sis_str_substr_nums(strval, ',');
	if (nums < 1 && tb->control.limits == 0)
	{
		tb->control.limits = 1;
	}
	for (int i=0; i < nums; i++) 
	{
		char mode[32];
		sis_str_substr(mode, 32, strval, ',', i);
		mm = sisdb_find_map_define(db_->map, mode, SIS_MAP_DEFINE_ADD_METHOD);
		if (mm)
		{
			tb->append_method |= mm->uid;
		}
	}
	//处理链接数据表名
	tb->publishs = sis_string_list_create_w();

	if (sis_json_cmp_child_node(com_, "publishs"))
	{
		strval = sis_json_get_str(com_, "publishs");
		sis_string_list_load(tb->publishs, strval, strlen(strval), ",");
	}

	//处理字段定义
	tb->field_name = sis_string_list_create_w();
	tb->field_map = sis_map_pointer_create();

	// 顺序不能变，必须最后
	sisdb_table_set_fields(tb, sis_json_cmp_child_node(com_, "fields"));
	// int count = sis_string_list_getsize(tb->field_name);
	// for (int i = 0; i < count; i++)
	// {
	// 	printf("---111  %s\n",sis_string_list_get(tb->field_name, i));
	// }

	s_sis_json_node *subs = sis_json_cmp_child_node(com_, "subscribe-method");
	if (subs)
	{
		tb->control.issubs = 1;
		s_sis_json_node *child = subs->child;
		while (child)
		{
			s_sisdb_field *fu = sisdb_field_get_from_key(tb, child->key);
			if (fu)
			{
				fu->subscribe_method = SIS_SUBS_METHOD_COPY;
				map = sisdb_find_map_define(db_->map, sis_json_get_str(child, "0"), SIS_MAP_DEFINE_SUBS_METHOD);
				if (map)
				{
					fu->subscribe_method = map->uid;
				}
				sis_strcpy(fu->subscribe_refer_fields, SIS_FIELD_MAXLEN, sis_json_get_str(child, "1"));
				// printf("%s==%d  %s--- %s\n", child->key, fu->subscribe_method, fu->subscribe_refer_fields,fu->name);
			}
			// printf("[%s] %s=====%p\n", tb->name, child->key, fu);
			child = child->next;
		}
	}

	s_sis_json_node *zip = sis_json_cmp_child_node(com_, "zip-method");
	if (zip)
	{
		tb->control.iszip = 1;
	}
	return tb;
}

void sisdb_table_destroy(s_sisdb_table *tb_)
//删除一个表
{
	// //删除字段定义
	s_sis_dict_entry *de;
	s_sis_dict_iter *di = sis_dict_get_iter(tb_->field_map);
	while ((de = sis_dict_next(di)) != NULL)
	{
		s_sisdb_field *val = (s_sisdb_field *)sis_dict_getval(de);
		sisdb_field_destroy(val);
	}
	sis_dict_iter_free(di);
	sis_map_pointer_destroy(tb_->field_map);

	sis_string_list_destroy(tb_->publishs);

	sis_string_list_destroy(tb_->field_name);
	
	sis_sdsfree(tb_->name);
	sis_free(tb_);
}

/////////////////////////////////////
//对数据库的各种属性设置
////////////////////////////////////

int sisdb_table_set_fields(s_sisdb_table *tb_, s_sis_json_node *fields_)
{
	int o = 0
	if (!fields_)
	{
		return o;
	}

	sis_map_pointer_clear(tb_->field_map);
	sis_string_list_clear(tb_->field_name);

	s_sis_json_node *node = sis_json_first_node(fields_);

	s_sisdb_field_flags flags;
	s_sis_map_define *map = NULL;
	int index = 0;
	int offset = 0;
	while (node)
	{
		// size_t ss;
		// printf("node=%s\n", sis_json_output(node, &ss));
		const char *name = sis_json_get_str(node, "0");

		flags.type = SIS_FIELD_TYPE_INT;
		const char *val = sis_json_get_str(node, "1");
		map = sisdb_find_map_define(tb_->father->map, val, SIS_MAP_DEFINE_FIELD_TYPE);
		if (map)
		{
			flags.type = map->uid;
		}

		flags.len = sis_json_get_int(node, "2", 4);
		if(flags.type==SIS_FIELD_TYPE_STRING||flags.type==SIS_FIELD_TYPE_JSON)
		{
			flags.len = sizeof(void *);
		}
		flags.dot = sis_json_get_int(node, "3", 0);

		s_sisdb_field *unit = sisdb_field_create(index++, name, &flags);
		unit->offset = offset;
		offset += unit->flags.len;
		// printf("[%d:%d] name=%s len=%d\n", index, offset, name, flags.len);
		sis_map_pointer_set(tb_->field_map, name, unit);
		sis_string_list_push(tb_->field_name, name, strlen(name));

		node = sis_json_next_node(node);
	}
	return o;
}

int sisdb_table_get_fields_size(s_sisdb_table *tb_)
{
	int len = 0;
	s_sis_dict_entry *de;
	s_sis_dict_iter *di = sis_dict_get_iter(tb_->field_map);
	while ((de = sis_dict_next(di)) != NULL)
	{
		s_sisdb_field *val = (s_sisdb_field *)sis_dict_getval(de);
		len += val->flags.len;
	}
	sis_dict_iter_free(di);
	return len;
}
