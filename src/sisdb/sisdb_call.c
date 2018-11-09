
#include <sisdb_io.h>
#include <sis_comm.h>

// 入口初始化，根据配置文件获取初始化所有信息，并返回s_sisdb_server指针，
// 一个实例只有一个server，但是可以有多个
char *call_sisdb_open(const char *conf_)
{
	if (!sis_file_exists(conf_))
	{
		sis_out_log(3)("conf file %s no finded.\n", conf_);
		return NULL;
	}
	return sisdb_open(conf_);
}

int call_sisdb_close(s_sis_module_context *ctx_, s_sis_module_string **argv_, int argc_)
{
	sis_module_not_used(argc_);
	sis_module_not_used(argv_);
	sisdb_close();
	return sis_module_reply_with_simple_string(ctx_, "OK");
}

// 显示表信息和其他系统级别的信息
int call_sisdb_show(s_sis_module_context *ctx_, s_sis_module_string **argv_, int argc_)
{
	sis_module_not_used(argc_);
	sis_module_not_used(argv_);

	s_sis_sds o = sisdb_show_sds(sis_module_string_get(argv_[1], NULL));
	if (o)
	{
		sis_module_reply_with_simple_string(ctx_, o);
		sis_sdsfree(o);
		return SIS_MODULE_OK;
	}
	return sis_module_reply_with_error(ctx_, "sisdb show table error.\n");
}

int call_sisdb_command(s_sis_module_context *ctx_, s_sis_module_string **argv_, int argc_)
{
	if (argc_ < 2)
	{
		return sis_module_wrong_arity(ctx_);
	}
	const char *command = sis_module_string_get(argv_[1], NULL);

	// 只能是先这样，后续需要建立指令map表，然后定位到函数进行处理
	// 包括一些定制的快速检索类功能
	if (!sis_strcasecmp(command, "init"))
	{
		const char *market = sis_module_string_get(argv_[2], NULL);
		sisdb_init(market);
	}
	else
	{
		return sis_module_reply_with_error(ctx_, "sisdb command no finded.\n");
	}
	return sis_module_reply_with_simple_string(ctx_, "OK");
}

// 获取数据可以根据 command中的 format来确定是json或者是struct
// 可以单独取数据头定义，比如fields等的定义
// 但保存在内存中的数据一定是二进制struct的数据格式，仅仅在输出时做数据格式转换
// set数据时也可以是json或struct格式数据，获得数据后会自动转换成不压缩的struct数据格式
int call_sisdb_get(s_sis_module_context *ctx_, s_sis_module_string **argv_, int argc_)
{
	if (argc_ < 2)
	{
		return sis_module_wrong_arity(ctx_);
	}

	s_sis_sds o;
	const char *key = sis_module_string_get(argv_[1], NULL);
	if (argc_ == 3)
	{
		o = sisdb_get_sds(key, sis_module_string_get(argv_[2], NULL));
	}
	else
	{
		o = sisdb_get_sds(key, "{\"format\":\"json\"}");
	}
	if (o)
	{
		sis_out_binary("get out", o, 30);
		printf("get out ...%lu\n", sis_sdslen(o));

		sis_module_reply_with_buffer(ctx_, o, sis_sdslen(o));
		sis_sdsfree(o);
		return SIS_MODULE_OK;
	}
	return sis_module_reply_with_error(ctx_, "sisdb get error.\n");
}
int call_sisdb_set(s_sis_module_context *ctx_, s_sis_module_string **argv_, int argc_)
{
	if (argc_ != 3)
	{
		return sis_module_wrong_arity(ctx_);
	}
	// printf("%s: %.90s\n", sis_module_string_get(argv_[1], NULL), sis_module_string_get(argv_[3], NULL));

	size_t len;
	const char *key = sis_module_string_get(argv_[1], NULL);
	const char *val = sis_module_string_get(argv_[2], &len);

	int o = sisdb_set_json(key, val, len);

	if (!o)
	{
		return sis_module_reply_with_simple_string(ctx_, "OK");
	}
	return sis_module_reply_with_error(ctx_, "sisdb set error.\n");
}

int call_sisdb_sset(s_sis_module_context *ctx_, s_sis_module_string **argv_, int argc_)
{
	if (argc_ != 3)
	{
		return sis_module_wrong_arity(ctx_);
	}
	// printf("%s: %.90s\n", sis_module_string_get(argv_[1], NULL), sis_module_string_get(argv_[3], NULL));

	size_t len;
	const char *key = sis_module_string_get(argv_[1], NULL);
	const char *val = sis_module_string_get(argv_[2], &len);

	int o = sisdb_set_struct(key, val, len);

	if (!o)
	{
		return sis_module_reply_with_simple_string(ctx_, "OK");
	}
	return sis_module_reply_with_error(ctx_, "sisdb sset error.\n");
}
int call_sisdb_save(s_sis_module_context *ctx_, s_sis_module_string **argv_, int argc_)
{
	sis_module_not_used(argc_);
	sis_module_not_used(argv_);

	if (sisdb_save())
	{
		return sis_module_reply_with_simple_string(ctx_, "OK");
	}
	return sis_module_reply_with_error(ctx_, "sisdb save error.\n");
}
int call_sisdb_out(s_sis_module_context *ctx_, s_sis_module_string **argv_, int argc_)
{
	if (argc_ < 1)
	{
		return sis_module_wrong_arity(ctx_);
	}

	bool o;
	if (argc_ == 3)
	{
		o = sisdb_out(
			sis_module_string_get(argv_[1], NULL),
			sis_module_string_get(argv_[2], NULL));
	}
	else
	{
		o = sisdb_out(sis_module_string_get(argv_[1], NULL),
					  "{\"format\":\"json\"}");
	}
	if (o)
	{
		return sis_module_reply_with_simple_string(ctx_, "OK");
	}
	return sis_module_reply_with_error(ctx_, "sisdb out error.\n");
}
int sis_module_on_unload()
{
	sis_out_log(3)("clsoe sisdb.\n");
	sisdb_close();
	safe_memory_stop();
	return SIS_MODULE_OK;
}
int sis_module_on_load(s_sis_module_context *ctx_, s_sis_module_string **argv_, int argc_)
{
	safe_memory_start();
	// 先取得服务名
	char *service_name;
	if (argc_ == 1)
	{
		service_name = call_sisdb_open(((s_sis_object *)argv_[0])->ptr);
	}
	else
	{
		service_name = call_sisdb_open("../sisdb/bin/sisdb.conf");
	}
	if (!service_name || !*service_name)
	{
		sis_out_log(3)("init sisdb error.\n");
		return SIS_MODULE_ERROR;
	}
	if (sis_module_init(ctx_, service_name, 1, SIS_MODULE_VER) == SIS_MODULE_ERROR)
		return SIS_MODULE_ERROR;

	/* Log the list of parameters passing loading the module. */
	// for (int k = 0; k < argc_; k++)
	// {
	// 	const char *s = sis_module_string_get(argv_[k], NULL);
	// 	printf("module loaded with argv_[%d] = %s\n", k, s);
	// }

	char command[64];
	sis_sprintf(command, 64, "%s.close", service_name);
	if (sis_module_create_command(ctx_, command, call_sisdb_close,
								  "readonly",
								  0, 0, 0) == SIS_MODULE_ERROR)
	{
		return SIS_MODULE_ERROR;
	}
	sis_sprintf(command, 64, "%s.show", service_name);
	if (sis_module_create_command(ctx_, command, call_sisdb_show,
								  "readonly",
								  0, 0, 0) == SIS_MODULE_ERROR)
	{
		return SIS_MODULE_ERROR;
	}
	sis_sprintf(command, 64, "%s.command", service_name);
	if (sis_module_create_command(ctx_, command, call_sisdb_command,
								  "write deny-oom",
								  0, 0, 0) == SIS_MODULE_ERROR)
	{
		return SIS_MODULE_ERROR;
	}
	sis_sprintf(command, 64, "%s.get", service_name);
	if (sis_module_create_command(ctx_, command, call_sisdb_get,
								  "readonly",
								  0, 0, 0) == SIS_MODULE_ERROR)
	{
		return SIS_MODULE_ERROR;
	}
	sis_sprintf(command, 64, "%s.set", service_name);
	if (sis_module_create_command(ctx_, command, call_sisdb_set,
								  "write deny-oom",
								  0, 0, 0) == SIS_MODULE_ERROR)
	{
		return SIS_MODULE_ERROR;
	}
	sis_sprintf(command, 64, "%s.sset", service_name);
	if (sis_module_create_command(ctx_, command, call_sisdb_sset,
								  "write deny-oom",
								  0, 0, 0) == SIS_MODULE_ERROR)
	{
		return SIS_MODULE_ERROR;
	}
	sis_sprintf(command, 64, "%s.save", service_name);
	if (sis_module_create_command(ctx_, command, call_sisdb_save,
								  "write deny-oom",
								  0, 0, 0) == SIS_MODULE_ERROR)
	{
		return SIS_MODULE_ERROR;
	}
	sis_sprintf(command, 64, "%s.out", service_name);
	if (sis_module_create_command(ctx_, command, call_sisdb_out,
								  "readonly",
								  0, 0, 0) == SIS_MODULE_ERROR)
	{
		return SIS_MODULE_ERROR;
	}
	return SIS_MODULE_OK;
}