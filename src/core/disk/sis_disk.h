﻿#ifndef _SIS_DISK_H
#define _SIS_DISK_H

#include "sis_disk.io.h"

////////////////////////////////////////////////////
// 外部调用接口定义
////////////////////////////////////////////////////

typedef struct s_sis_disk_reader_cb {
    // 用户需要传递的上下文
    void *cb_source; 
    // 通知文件开始读取了或订阅开始 需要全部转换为毫秒传出 无时序为0
    void (*cb_start)(void *, int);  // 日期
    // 返回key定义
    void (*cb_dict_keys)(void *, void *key_, size_t);  // 返回key信息
    // 返回sdb定义
    void (*cb_dict_sdbs)(void *, void *sdb_, size_t);  // 返回属性信息
    // 返回解压后的数据(可能一条或多条数据)
    // 支持 SDB SNO 不支持 LOG IDX
    void (*cb_userdate)(void *, const char *kname_, const char *sname_, void *out_, size_t olen_); 
    // 返回解压后的数据(可能一条或多条数据)
    // 支持 SDB SNO 不支持 LOG IDX
    void (*cb_realdate)(void *, int kidx_, int sidx_, void *out_, size_t olen_); 
    // 返回原始头标记后的原始数据(未解压) PACK时只能返回该数据结构 速度快
    // 支持 LOG SNO SDB IDX 
    void (*cb_original)(void *, s_sis_disk_head *, void *out_, size_t olen_); // 头描述 实际返回的数据区和大小
    // 通知文件结束了
    void (*cb_stop)(void *, int); //第一个参数就是用户传递进来的指针
} s_sis_disk_reader_cb;


// 通用的读取数据函数 如果有字典数据key就用字典的信息 
// 如果没有字典信息就自己建立一个
typedef struct s_sis_disk_writer {
    // ------input------ //
    int                   style;
    s_sis_sds             fpath;
    s_sis_sds             fname;
    // ------contorl------ //
    int                   status;     // 状态
    s_sis_disk_ctrl      *munit;      // 非时序写入类 sno和log和sdb
    s_sis_map_list       *units;      // sdb 的按时序存储的结构映射表
    // 一组操作类 s_sis_disk_ctrl 适用于sdb的时序数据
    // 日时序数据 以起始年为索引
    // 其他时序以日期为索引 
	s_sis_map_list       *map_keys;   // s_sis_sds
	s_sis_map_list       *map_sdbs;   // 全部表结构 s_sis_dynamic_db
	s_sis_map_list       *map_year;   // 日上时序数据表指针 *s_sis_dynamic_db
	s_sis_map_list       *map_date;   // 日下时序数据表指针 *s_sis_dynamic_db

} s_sis_disk_writer;


typedef struct s_sis_disk_reader {
    // ------input------ //
    s_sis_sds             fpath;
    s_sis_sds             fname;
    s_sis_msec_pair       search_msec;
    s_sis_sds             sub_keys;   // "*" 为全部都要 或者 k1,k2,k3
	s_sis_sds             sub_sdbs;   // "*" 为全部都要 或者 k1,k2,k3
    s_sis_disk_reader_cb *callback;   // 读文件的回调

    // ------contorl------ //
    int                   status;     // 0 未初始化 1 可以订阅 2 不能订阅
	s_sis_map_list       *map_keys;   // s_sis_sds
	s_sis_map_list       *map_sdbs;   // 全部表结构 s_sis_dynamic_db
	s_sis_map_list       *map_year;   // 日上时序数据表指针 *s_sis_dynamic_db
	s_sis_map_list       *map_date;   // 日下时序数据表指针 *s_sis_dynamic_db

    uint8                 issub;      // 是否为订阅 不是订阅就一次性返回数据
                                      // 如果是订阅 根据whole判断是否整块输出
    uint8                 iswhole;    // 是否一次性输出 
}s_sis_disk_reader;


///////////////////////////
//  s_sis_disk_writer
///////////////////////////
// 如果目录下已经有不同类型文件 返回错误
s_sis_disk_writer *sis_disk_writer_create(const char *path_, const char *name_, int style_);
void sis_disk_writer_destroy(void *);

// LOG SNO 为文件实际时间 SDB 为文件修改日期 
int sis_disk_writer_open(s_sis_disk_writer *, int idate_);
// 关闭所有文件 重写索引
void sis_disk_writer_close(s_sis_disk_writer *);
// 所有基础类和子类文件key保持最新一致 但仍然有可能老的子类文件不一致
void sis_disk_writer_kdict_changed(s_sis_disk_writer *writer_, const char *kname_);
// 基础类为结构全集，子类文件只保留当前时序的结构说明
void sis_disk_writer_sdict_changed(s_sis_disk_writer *writer_, const char *sname_, s_sis_dynamic_db *db_);
// 写入键值信息 - 可以多次写 新增的添加到末尾 仅支持 SNO SDB
int sis_disk_writer_set_kdict(s_sis_disk_writer *, const char *in_, size_t ilen_);
// 设置表结构体 - 根据不同的时间尺度设置不同的标记 仅支持 SNO SDB
int sis_disk_writer_set_sdict(s_sis_disk_writer *, const char *in_, size_t ilen_);

//////////////////////////////////////////
//                  log 
//////////////////////////////////////////
// 写入数据 仅支持 LOG 不管数据多少 直接写盘 
// sis_disk_writer_open
// sis_disk_writer_log
// sis_disk_writer_close
size_t sis_disk_writer_log(s_sis_disk_writer *, void *in_, size_t ilen_);

//////////////////////////////////////////
//                  sno 
//////////////////////////////////////////
// 开始写入数据  支持SNO 
int sis_disk_writer_start(s_sis_disk_writer *);
// 数据传入结束 剩余全部写盘 支持SNO
void sis_disk_writer_stop(s_sis_disk_writer *);
// 写入数据 仅支持 SNO 必须是原始数据
// sis_disk_writer_open
// sis_disk_writer_set_kdict
// sis_disk_writer_set_sdict
// sis_disk_writer_start
// sis_disk_writer_sno
// ....
// sis_disk_writer_stop
// sis_disk_writer_close
int sis_disk_writer_sno(s_sis_disk_writer *, const char *kname_, const char *sname_, void *in_, size_t ilen_);
//////////////////////////////////////////
//  sdb 
//////////////////////////////////////////
// 写入标准数据 kname 如果没有就新增 sname 必须字典已经有了数据 
// 需要根据数据的时间字段 确定对应的文件
// sis_disk_writer_open
// sis_disk_writer_set_kdict
// sis_disk_writer_set_sdict
// sis_disk_writer_sdb
// ...
// sis_disk_writer_one
// ...
// sis_disk_writer_mul
// ...
// sis_disk_writer_close
// 结构化时序和无时序数据 
size_t sis_disk_writer_sdb(s_sis_disk_writer *, const char *kname_, const char *sname_, void *in_, size_t ilen_);
// 单键值单记录数据 
size_t sis_disk_writer_one(s_sis_disk_writer *, const char *kname_, void *in_, size_t ilen_);
// 单键值多记录数据 inlist_ : s_sis_sds 的列表
size_t sis_disk_writer_mul(s_sis_disk_writer *, const char *kname_, s_sis_pointer_list *inlist_);

///////////////////////////
//  s_sis_disk_reader
///////////////////////////
s_sis_disk_reader *sis_disk_reader_create(const char *path_, const char *name_, s_sis_disk_reader_cb *cb_);
void sis_disk_reader_destroy(void *);

// 打开 准备读 首先加载IDX到内存中 就知道目录下支持哪些数据了 LOG SNO SDB
int sis_disk_reader_open(s_sis_disk_reader *);
// 关闭所有文件 设置了不同订阅条件后可以重新
void sis_disk_reader_close(s_sis_disk_reader *);

// 从对应文件中获取数据 拼成完整的数据返回 只支持 SDB 单键单表
// 多表按时序输出通过该函数获取全部数据后 排序输出
s_sis_object *sis_disk_reader_get_obj(s_sis_disk_reader *, const char *kname_, const char *sname_, s_sis_msec_pair *smsec_);

// 以流的方式读取文件 从文件中一条一条发出 按时序 无时序的会最先发出 只支持 SDB 
// 可支持多个key和sdb订阅 k1,k2,k3  db1,db2,db3
// 按磁盘存储的块 所有键无时序的先发 然后有时序读取第一块 然后排序返回 依次回调 cb_realdate 直到所有数据发送完毕 
int sis_disk_reader_sub(s_sis_disk_reader *, const char *keys_, const char *sdbs_, s_sis_msec_pair *smsec_);

// 顺序读取 仅支持 LOG  通过回调的 cb_original 返回数据
int sis_disk_reader_sub_log(s_sis_disk_reader *, int idate_);

// 顺序读取 仅支持 SNO  通过回调的 cb_original 或 cb_realdate 返回数据
// 如果定义了 cb_realdate 就解压数据再返回
// 可支持多个key和sdb订阅 k1,k2,k3  db1,db2,db3
int sis_disk_reader_sub_sno(s_sis_disk_reader *, const char *keys_, const char *sdbs_, int idate_);

// 取消一个正在订阅的任务 只有处于非订阅状态下才能订阅 避免重复订阅
int sis_disk_reader_unsub(s_sis_disk_reader *);

///////////////////////////
//  s_sis_disk_control
///////////////////////////
// 不论该目录下有任何类型文件 全部删除
int sis_disk_control_clear(const char *path_, const char *name_);


#endif

