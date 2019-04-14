﻿
#ifndef _SIS_NET_H
#define _SIS_NET_H

#include <os_net.h>
#include <sis_thread.h>
#include <sis_malloc.h>
#include <sis_str.h>
#include <sis_net.node.h>

#define SIS_NET_INIT			1  //初始化后的状态
#define SIS_NET_ERROR			2  //初始化没有的协议
#define SIS_NET_EXIT		    0  //准备退出，通知线程结束工作

#define SIS_NET_CONNECTED    3  //正常工作
#define SIS_NET_CONNECTING   4  //连接前设置状态...
#define SIS_NET_WAITCONNECT  5  //等待连接的状态 连接失败或者连接断开
#define SIS_NET_CONNECTSTOP  6  //不再发起连接

// 网络通讯时的角色
#define SIS_NET_ROLE_NONE   0  //
#define SIS_NET_ROLE_REQ    (1<<0)  //A在该状态下为请求端，主动发一个请求到B，
#define SIS_NET_ROLE_ANS    (1<<1)  //A在该状态下为应答端，等待接收一条信息，
#define SIS_NET_ROLE_SUB    (1<<2)  //A在该状态下处于循环接收数据的状态，
#define SIS_NET_ROLE_PUB    (1<<3)  //A在该状态下处于不断发送数据的状态，
//#define SIS_NET_ROLE_REG    (1<<4)  //A在该状态下处于注册服务的状态，接收消息并发送信息

#define SIS_NET_SIGN_MAX_LEN  128

typedef struct s_sis_url {
    char protocol[SIS_NET_SIGN_MAX_LEN];  //protocol -- 目前只支持 redis / tcp / file;
    char ip[SIS_NET_SIGN_MAX_LEN];  
                        //ip 对方ip地址 为*表示本机为服务器，等待连接
						//redis://192.168.1.202:3679   从redis上获取数据
						//mongodb://192.168.1.202:3679  从redis上获取数据
						//owner://SH:0  从market列表选取code为SH的数据接口，0备用
    int  port;  // 端口号
    bool auth;  // 是否需要用户验证
    char username[SIS_NET_SIGN_MAX_LEN];   // username 
    char password[SIS_NET_SIGN_MAX_LEN];   // 密码
	// -------------- //
	// 下面应该建立一个key-value的对照表，作为指针存在，方便存储各种数据
	char dbname[SIS_NET_SIGN_MAX_LEN];  // 数据库名或文件名  

	char workpath[SIS_PATH_LEN];  // 需要处理的路径名  
	char workfile[SIS_PATH_LEN];   // 需要处理的文件名  
} s_sis_url;

typedef struct s_sis_socket {
	s_sis_url    url;
	int          rale;
	int          flags;    //当前状态  req 等待发请求, ans 发完请求等待应答, 应答接收后状态修改为req， 
						   //订阅状态，sub 只接受数据; pub只发送数据, ans可以同时存在；
						   //  - client    SIS_NET_ROLE_REQ | SIS_NET_ROLE_SUB 发完请求后处于订阅状态
						   //  - service   SIS_NET_ROLE_REQ | SIS_NET_ROLE_SUB | SIS_NET_ROLE_PUB 
	int    status;         //当前的网络连接状态 SIS_NET_CONNECTED...

	s_sis_mutex_rw  mutex_message_send;    //发送队列锁
	s_sis_list     *message_send;   /* 需要处理的发送信息列表 */

	s_sis_mutex_rw  mutex_message_recv;    //接收信息锁
	s_sis_list     *message_recv;  /* 需要处理的接收到的信息列表 */

	void          *handle_read;    //通过该句柄发出的消息不等待回应，由线程接受信息，线程循环接受redis发布过来的信息，然后写入message_recv，
	int            status_read;    //当前的状态；
	s_sis_mutex_rw mutex_read;

	void          *handle_write; //一应一答的通用操作模式
	int            status_write; //当前的状态；
	s_sis_mutex_rw mutex_write;

	s_sis_list *pub_keys;  /* 其他人订阅我的key */
	s_sis_list *sub_keys;  /* 我要订阅别人的key */

	bool fastwork_mode; //快速通道模式
	int  fastwork_count;//记录数
	int  fastwork_bytes;//字节数

	//仅仅向服务器发送信息，不需要返回响应信息
	bool(*socket_send_message)(struct s_sis_socket *, s_sis_message_node *);
	//发信息并等待服务器响应
	s_sis_message_node * (*socket_query_message)(struct s_sis_socket *, s_sis_message_node *); 

	void(*open)(struct s_sis_socket *);
	void(*close)(struct s_sis_socket *);
	
} s_sis_socket;


s_sis_socket *sis_socket_create(s_sis_url *url_,int rale_); //rale_ 表示身份
void sis_socket_destroy(void *sock_);

void sis_socket_open(s_sis_socket *sock_);
void sis_socket_close(s_sis_socket *sock_);

bool sis_socket_send_message(s_sis_socket *sock_, s_sis_message_node *mess_);
s_sis_message_node *sis_socket_query_message(s_sis_socket *sock_, s_sis_message_node *mess_);


// #define SIS_NET_PROTOCOL_TYPE  "redis,tcp,http,https,ws,wss"
// #define SIS_NET_PROTOCOL_REDIS  0
// #define SIS_NET_PROTOCOL_TCP    1
// #define SIS_NET_PROTOCOL_HTTP   2
// #define SIS_NET_PROTOCOL_HTTPS  3
// #define SIS_NET_PROTOCOL_WS     4
// #define SIS_NET_PROTOCOL_WSS    5

// 	int style = sis_str_subcmp(url_->protocol,SIS_NET_PROTOCOL_TYPE,',');
// 	if (style < 0) {
// 		return NULL;
// 	}
//     	switch (style)
// 	{
// 		case SIS_NET_PROTOCOL_REDIS:
// 			sock->open = sis_redis_open;
// 			sock->close = sis_redis_close;

// 			sock->socket_send_message = sis_redis_send_message;
// 			sock->socket_query_message = sis_redis_query_message;
// 			break;	
// 		default:
// 			sock->status = SIS_NET_ERROR;
// 			break;
// 	}

// 两种选择：
// 一种是完全采用redis协议格式，
// 一种是
// 内部通讯协议为redis格式，js外部协议为json，依靠驱动进行数据序列化处理；

// 通过驱动来转换数据格式，同时负责解压加密等工作，最终返回给js前端的数据格式为json格式数据。

// 外部请求格式:
// id  -- 谁干的事结果给谁
// com -- 干什么
// key -- 对谁干
// argv -- 怎么干...

// xxx:{"cmd":"sisdb.get","key":"sh6006001.info","argv":[11,"sssss",{"#":16}]}

// {"ID":"xxxx","com":"sisdb.get","key":"sh6006001.info","argv":[11,"sssss",(16)]}....16个字节的二进制数据 
// 如果参数为二进制数据，就以()为标志，中间放数据长度，json数据后以 0x0为分隔符，依次放入二进制数据，形成完整的数据包，


// 应答部分格式:
// +  --  字符串 
// -  --  错误
// :  --  整数
// $  --  数组 后面为*列表
// *  --  长度 后面为缓存区

// [service]:
// 	所有的消息都以字符串或数字为开始，以冒号为结束；
// 	system:为系统类消息，比如链接、登录、PING等信息；
// 	不放在{}中的原因是解析简单，快速定位转发
// 	1.作为转发中间件时，aaa.bbb.ccc的方式一级一级下去，返回数据后再一级一级上来，最后返回给发出请求的端口，
// 	  方便做分布式应用的链表；因为该值会有变化，和请求数据分开放置；只有内存拷贝没有json解析
// 	2.作为订阅和发布，当客户订阅了某一类数据，异步返回数据时可以准确的定位数据源，
// 	3.对于一应一答的阻塞请求，基本可以忽略不用；
//     4.对于客户端发出多个请求，服务端返回数据时以stock来定位请求；

// https://blog.csdn.net/u014608280/article/details/84586042

// *3\r\n
// $5\r\n
// 12345\r\n
// $9\r\n
// sisdb.get\r\n
// $13\r\n
// sh600601.info\r\n


// 冒号结束后为消息体:
// [com]
// [key]
// [arg] {binary:1}



#endif //_SIS_NET_H
