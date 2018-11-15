
#include <sis_csv.h>
#include <sis_memory.h>

int _sis_file_csv_parse(s_sis_file_csv *csv_)
{
	if (!csv_ || !csv_->fp) return  0;
	
	sis_struct_list_clear(csv_->list);

    s_sis_memory *buffer = sis_memory_create();
    while (1)
    {
        size_t bytes = sis_memory_readfile(buffer, csv_->fp, SIS_DB_MEMORY_SIZE);
        if (bytes <= 0)
            break;
		size_t offset = sis_memory_get_line_sign(buffer); 
		// 偏移位置包括回车字符 0 表示没有回车符号，需要继续读
        while (offset)
        {
			s_sis_string_list *str = sis_string_list_create_w();
			sis_string_list_load(str, sis_memory(buffer), offset, ",");
			if(sis_string_list_getsize(str) > 0 ) {
				sis_struct_list_push(csv_->list, str);
			} else {
				sis_string_list_destroy(str);
			}
			sis_memory_move(buffer, offset);
			offset = sis_memory_get_line_sign(buffer); 
        }
    }
    sis_memory_destroy(buffer);

	return csv_->list->count;
}
s_sis_file_csv * sis_file_csv_open(const char *name_,int mode_, int access_)
{
	mode_ = (!mode_) ? SIS_FILE_IO_READ : mode_;
    sis_file_handle fp = sis_file_open(name_, mode_, access_);
    if (!fp)
    {
		return NULL;
    }

    s_sis_file_csv * o =sis_malloc(sizeof(s_sis_file_csv));
    memset(o, 0, sizeof(s_sis_file_csv));
	o->sign[0] = ',', o->sign[1] = 0;
	o->list = sis_pointer_list_create();
	o->list->free = sis_string_list_destroy;
	o->fp = fp;
	
	_sis_file_csv_parse(o);

	sis_file_close(o->fp);
    return o;
}
void sis_file_csv_close(s_sis_file_csv *csv_)
{
	if(!csv_) return ;
	sis_pointer_list_destroy(csv_->list);
	sis_free(csv_);
}

size_t sis_file_csv_read(s_sis_file_csv *csv_, char *in_, size_t ilen_)
{
	// if (!csv_ || !csv_->fp) return  0;
	return sis_file_read(csv_->fp, in_, 1, ilen_);
}
size_t sis_file_csv_write(s_sis_file_csv *csv_, char *in_, size_t ilen_)
{
	// if (!csv_ || !csv_->fp) return  0;
	return sis_file_write(csv_->fp, in_, 1, ilen_);
}

s_sis_sds sis_file_csv_get(s_sis_file_csv *csv_, char *key_)
{
	return NULL;
}
size_t sis_file_csv_set(s_sis_file_csv *csv_, char *key_, char *in_, size_t ilen_)
{
	return 0;
}

#if 0

// no test ,need test!

#endif
