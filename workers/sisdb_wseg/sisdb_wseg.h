﻿#ifndef _sisdb_wseg_h
#define _sisdb_wseg_h

#include "sis_method.h"
#include "sis_disk.h"
#include "sisdb_incr.h"

#define  SIS_WSEG_NONE     0 // 
#define  SIS_WSEG_INIT     1 // 是否初始化
#define  SIS_WSEG_OPEN     2 // 是否打开
#define  SIS_WSEG_HEAD     3 // 是否写了头
#define  SIS_WSEG_EXIT     4 // 退出
#define  SIS_WSEG_FAIL     5 // 打开失败

// 一种分离式sno的读写格式
// 对写入的数据格式 分为不同时间节点 写入不同文件 方便不同时间需求的数据存取
// 日时序 date/2020/20201010.idx
//       date/2020/20201010.sno
//       日时序的数据结构体可以key设置为CN000000 所有股票的集合体 方便读取 也可以同时存放单个股票的数据
// 分时序 minu/2020/20201010.idx
//       minu/2020/20201010.sno
// 秒时序 msec/2020/20201010.idx
//       msec/2020/20201010.sno
// 无时序 none/2020/20201010.idx
//       none/2020/20201010.sno

#define  SIS_SEG_DATE  0
#define  SIS_SEG_MINU  1
#define  SIS_SEG_MSEC  2
#define  SIS_SEG_NONE  3

typedef struct s_sisdb_wseg_cxt
{
    int                status;
    s_sis_sds_save    *work_path;     // 可配置 也可传入
    s_sis_sds_save    *work_name;     // 可配置 也可传入

    s_sis_disk_writer *writer[4];     // 写盘类
 
	int                work_date;     // 工作日期  
	s_sis_sds          work_keys;     // 筛选后的 
	s_sis_sds          work_sdbs;     // 筛选后的

	s_sis_map_list    *maps_sdbs;     // 表对应的map s_sis_dynamic_db

	s_sis_sds          wsno_keys;     // 外部写入的keys
	s_sis_sds          wsno_sdbs;     // 外部写入的sdbs

	s_sisdb_incr      *work_unzip;    // 解压 s_snodb_compress 中来的数据

}s_sisdb_wseg_cxt;

bool sisdb_wseg_init(void *, void *);
void sisdb_wseg_uninit(void *);

int cmd_sisdb_wseg_getcb(void *worker_, void *argv_);

void sisdb_wseg_clear(s_sisdb_wseg_cxt *context);
const char *sisdb_wseg_get_sname(int idx);
int sisdb_wseg_get_style(s_sis_dynamic_db *db);

#endif