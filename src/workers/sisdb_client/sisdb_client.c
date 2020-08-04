﻿#include "sis_modules.h"
#include "worker.h"
#include "server.h"
#include "sis_method.h"

#include "sisdb_client.h"
#include "sisdb.h"
#include "sis_net.io.h"
// #include "sisdb_server.h"
// #include "sisdb_collect.h"
///////////////////////////////////////////////////
// *** s_sis_modules sis_modules_[dir name]  *** //
///////////////////////////////////////////////////

struct s_sis_method sisdb_client_methods[] = {
    {"send", cmd_sisdb_client_send, NULL, NULL},  // json 格式
};
// 通用文件存取接口
s_sis_modules sis_modules_sisdb_client = {
    sisdb_client_init,
    NULL,
    NULL,
    NULL,
    sisdb_client_uninit,
    sisdb_client_method_init,
    sisdb_client_method_uninit,
    sizeof(sisdb_client_methods) / sizeof(s_sis_method),
    sisdb_client_methods,
};

s_sisdb_client_ask *sisdb_client_ask_create(
    const char   *cmd_,            // 请求的参数
	const char   *key_,            // 请求的key
	const char   *val_,            // 请求的参数
	void         *cb_source_,          // 回调传送对象
    void         *cb_sub_start,        // 回调开始
	void         *cb_sub_realtime,     // 订阅进入实时状态
	void         *cb_sub_stop,         // 订阅结束 自动取消订阅
	void         *cb_reply             // 回调的数据
)
{
    s_sisdb_client_ask *ask = SIS_MALLOC(s_sisdb_client_ask, ask);
    ask->cmd = sis_sdsnew(cmd_);
    ask->key = sis_sdsnew(key_);
    ask->val = sis_sdsnew(val_);
    ask->cb_source = cb_source_;
    ask->cb_sub_start = cb_sub_start;
    ask->cb_sub_realtime = cb_sub_realtime;
    ask->cb_sub_stop = cb_sub_stop;
    ask->cb_reply = cb_reply;
    return ask;
}

void sisdb_client_ask_destroy(void *ask_)
{
    s_sisdb_client_ask *ask = (s_sisdb_client_ask *)ask_;
    sis_sdsfree(ask->cmd);
    sis_sdsfree(ask->key);
    sis_sdsfree(ask->val);
    sis_free(ask);
}

bool sisdb_client_init(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sis_json_node *node = (s_sis_json_node *)argv_;

    s_sisdb_client_cxt *context = SIS_MALLOC(s_sisdb_client_cxt, context);
    worker->context = context;

    sis_url_set_ip4(&context->url_cli, sis_json_get_str(node, "ip"));
    context->url_cli.port = sis_json_get_int(node, "port", 7329);
    context->url_cli.io = SIS_NET_IO_CONNECT;
    context->url_cli.role = SIS_NET_ROLE_REQUEST;
    context->url_cli.version = 1;
    context->url_cli.protocol = SIS_NET_PROTOCOL_WS;
    context->url_cli.compress = 0;
    context->url_cli.crypt = 0;
    context->url_cli.dict = NULL;

    sis_strcpy(context->username, 32, sis_json_get_str(node, "username"));
    sis_strcpy(context->password, 32, sis_json_get_str(node, "password"));
    if(sis_strlen(context->username) > 0 && sis_strlen(context->password) > 0)
    {
        context->auth = true;
    }

    context->ask_sno = 0; 

    context->asks = sis_map_pointer_create_v(sisdb_client_ask_destroy);

    context->status = SIS_CLI_STATUS_INIT;
    // printf("%s %p, %d\n", __func__, context, context->status);
    return true;
}
void sisdb_client_uninit(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_client_cxt *context = (s_sisdb_client_cxt *)worker->context;
    sis_map_pointer_destroy(context->asks);
    sis_free(context);

}


void sisdb_client_send_ask(s_sisdb_client_cxt *context, s_sisdb_client_ask *ask)
{
    s_sis_net_message *msg = sis_net_message_create();
    msg->cid = context->cid;
    msg->source = sis_sdsnew(ask->source);
    sis_net_ask_with_chars(msg, ask->cmd, ask->key, ask->val, sis_sdslen(ask->val));
    sis_net_class_send(context->session, msg);
    sis_net_message_destroy(msg);
}

static void _cb_recv(void *source_, s_sis_net_message *msg_)
{
    s_sisdb_client_cxt *context = (s_sisdb_client_cxt *)source_;
    if (context->status == SIS_CLI_STATUS_WORK)
    {
        s_sisdb_client_ask *ask = sisdb_client_ask_get(context, msg_->source);
        if (ask && msg_->style & SIS_NET_ANS_MSG)
        {
            if (msg_->style & SIS_NET_ANS_INT)
            {
                if(ask->cb_reply)
                {
                    char info[32];
                    sis_llutoa(msg_->rint, info, 32, 10);
                    ask->cb_reply(ask->cb_source, info);
                }
            }
            else if (msg_->style & SIS_NET_ANS_ARGVS)
            {
                //
            }
            else if (msg_->style & SIS_NET_ANS_VAL)
            {
                if(ask->cb_reply)
                {
                    ask->cb_reply(ask->cb_source, msg_->rval);
                }                
            }
            else if (msg_->style & SIS_NET_ANS_SIGN)
            {
                switch (msg_->rint)
                {
                case SIS_NET_ANS_SIGN_SUB_START:
                    if(ask->cb_sub_start)
                    {
                        ask->cb_sub_start(ask->cb_source, msg_->rval);
                    }        
                    break;
                case SIS_NET_ANS_SIGN_SUB_WAIT:
                    if(ask->cb_sub_realtime)
                    {
                        ask->cb_sub_realtime(ask->cb_source, msg_->rval);
                    }        
                    break;
                case SIS_NET_ANS_SIGN_SUB_STOP:
                    if(ask->cb_sub_stop)
                    {
                        ask->cb_sub_stop(ask->cb_source, msg_->rval);
                    } 
                    // 取消订阅
                    {
                        char ds[128], cmd[128]; 
                        sis_str_divide(ask->cmd, '.', ds, cmd);

                        s_sis_net_message *msg = sis_net_message_create();
                        msg->cid = context->cid;
                        if (!sis_strcasecmp("sub", cmd))
                        {
                            sis_net_ask_with_chars(msg, "unsub", ask->key, ask->val, sis_sdslen(ask->val));
                        }
                        else
                        {
                            sis_net_ask_with_chars(msg, "unsubsno", ask->key, ask->val, sis_sdslen(ask->val));
                        }                        
                        sis_net_class_send(context->session, msg);
                        sis_net_message_destroy(msg);
                        // 取消订阅后 删除本地信息
                        sisdb_client_ask_del(context, ask);
                    }

                    break;
                case SIS_NET_ANS_SIGN_OK:
                    if(ask->cb_reply)
                    {
                        // 其他返回NULL
                        ask->cb_reply(ask->cb_source, NULL);
                        // 返回 NULL 表示可不理会返回值 需要错误信息调用函数
                    }        
                    break;
                case SIS_NET_ANS_SIGN_NIL:
                case SIS_NET_ANS_SIGN_ERROR:
                default:
                    if(ask->cb_reply)
                    {
                        // 其他返回NULL
                        ask->cb_reply(ask->cb_source, NULL);
                        // 返回 NULL 表示可不理会返回值 需要错误信息调用函数
                        if (context->info)
                        {
                            sis_sdsfree(context->info);
                        }
                        context->info = sis_sdsdup(msg_->rval);
                    }        
                    break;
                }
            }
            // 已经处理完数据 清理ask
            if (!ask->issub)
            {
                sisdb_client_ask_del(context, ask);
            }
        }
    }
    if (context->status == SIS_CLI_STATUS_AUTH)
    {
        // 判断是否验证通过
        if(msg_->style == SIS_NET_ANS_SIGN && msg_->rint == SIS_NET_ANS_SIGN_OK)
        {        
            // 发送订阅信息
            s_sis_dict_entry *de;
            s_sis_dict_iter *di = sis_dict_get_iter(context->asks);
            while ((de = sis_dict_next(di)) != NULL)
            {
                s_sisdb_client_ask *ask = (s_sisdb_client_ask *)sis_dict_getval(de);
                if (ask->issub)
                {
                    sisdb_client_send_ask(context, ask);
                }
            }
            sis_dict_iter_free(di);
            // 设置工作状态
            context->status = SIS_CLI_STATUS_WORK;
        }
        else
        {
            LOG(5)("auth fail.\n");
        }
    }    	
}

static void _cb_connected(void *source_, int sid)
{
    s_sisdb_client_cxt *context = (s_sisdb_client_cxt *)source_;

    sis_net_class_set_cb(context->session, sid, context, _cb_recv);

    // 连接后判断是否需要发送登录密码 
    // printf("%s %p, %d %d\n", __func__, context, context->status, sid);
    if (context->status != SIS_CLI_STATUS_WORK && context->status != SIS_CLI_STATUS_EXIT)
    {
        if (context->auth)
        {
            // 发送验证信息
            context->status = SIS_CLI_STATUS_AUTH;
            context->cid = sid;
            s_sis_net_message *msg = sis_net_message_create();
            msg->cid = context->cid;
            sis_net_ask_with_chars(msg, "auth", context->username, context->password, sis_strlen(context->password));
            sis_net_class_send(context->session, msg);
            sis_net_message_destroy(msg);
        }
        else
        {
            context->status = SIS_CLI_STATUS_WORK;
        }
    }
}
static void _cb_disconnect(void *source_, int sid)
{
    s_sisdb_client_cxt *context = (s_sisdb_client_cxt *)source_;
    // 断开连接 
    sis_net_class_set_cb(context->session, sid, context, NULL);
    
    context->status = SIS_CLI_STATUS_INIT;
}
void sisdb_client_method_init(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_client_cxt *context = (s_sisdb_client_cxt *)worker->context;

    context->session = sis_net_class_create(&context->url_cli);

    if (context->session)
    {
        context->session->cb_source = context;
        context->session->cb_connected = _cb_connected;
        context->session->cb_disconnect = _cb_disconnect;

        sis_net_class_open(context->session);
    } 
}
void sisdb_client_method_uninit(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_client_cxt *context = (s_sisdb_client_cxt *)worker->context;
    context->status = SIS_CLI_STATUS_EXIT;
    if (context->session)
    {
        sis_net_class_close(context->session);
        sis_net_class_destroy(context->session);
        context->session = NULL;
    }
}
s_sisdb_client_ask *sisdb_client_ask_new(s_sisdb_client_cxt *context, 
    const char   *cmd_,            // 请求的参数
	const char   *key_,                // 请求的key
	const char   *val_,                // 请求的参数
	void         *cb_source_,          // 回调传送对象
    void         *cb_sub_start,        // 回调开始
	void         *cb_sub_realtime,     // 订阅进入实时状态
	void         *cb_sub_stop,         // 订阅结束 自动取消订阅
	void         *cb_reply,            // 回调的数据
    bool          issub)
{
    s_sisdb_client_ask *ask = sisdb_client_ask_create(cmd_, key_, val_, cb_source_, cb_sub_start, cb_sub_realtime, cb_sub_stop, cb_reply);
    ask->issub = issub;
    context->ask_sno = (context->ask_sno + 1 ) % 0xFFFFFFFF;
    sis_llutoa(context->ask_sno, ask->source, 16, 10);
    ask->issub = issub;
    sis_map_pointer_set(context->asks, ask->source, ask);
    return ask;
}
void sisdb_client_ask_del(s_sisdb_client_cxt *context, s_sisdb_client_ask *ask_)
{
    if (ask_)
    {
        s_sisdb_client_ask *ask = sis_map_pointer_get(context->asks, ask_->source);
        if (ask)
        {
            sis_map_pointer_del(context->asks, ask->source);
        }
        else
        {
            sisdb_client_ask_destroy(ask_);
        }      
    }
}

s_sisdb_client_ask *sisdb_client_ask_get(
    s_sisdb_client_cxt *context, 	
    const char   *source         // 来源信息
)
{
    return (s_sisdb_client_ask *)sis_map_pointer_get(context->asks, source);
}

void sisdb_client_ask_unsub(
    s_sisdb_client_cxt *context, 	
    const char   *cmd_,         // 来源信息
    const char   *key_         // 来源信息
)
{
    char *str = NULL;
    s_sis_dict_entry *de;
    s_sis_dict_iter *di = sis_dict_get_iter(context->asks);
    while ((de = sis_dict_next(di)) != NULL)
    {
        s_sisdb_client_ask *ask = (s_sisdb_client_ask *)sis_dict_getval(de);
        if (ask->issub && !sis_strcasecmp(ask->cmd, cmd_) && !sis_strcasecmp(ask->key, key_) )
        {
            str = ask->source;
            break;
        }
    }
    sis_dict_iter_free(di);
    if (str)
    {
        sis_map_pointer_del(context->asks, str);
    }
}

bool sisdb_client_ask_sub_exists(
    s_sisdb_client_cxt *context, 	
    const char   *cmd_,         // 来源信息
    const char   *key_         // 来源信息
)
{
    bool exists = false;
    s_sis_dict_entry *de;
    s_sis_dict_iter *di = sis_dict_get_iter(context->asks);
    while ((de = sis_dict_next(di)) != NULL)
    {
        s_sisdb_client_ask *ask = (s_sisdb_client_ask *)sis_dict_getval(de);
        if (ask->issub && !sis_strcasecmp(ask->cmd, cmd_) && !sis_strcasecmp(ask->key, key_) )
        {
            exists = true;
            break;
        }
    }
    sis_dict_iter_free(di);
    return exists;
}
int cmd_sisdb_client_send(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_client_cxt *context = (s_sisdb_client_cxt *)worker->context;

    printf("%s %d\n",__func__, context->status);
    if (!argv_ || context->status != SIS_CLI_STATUS_WORK)
    {
        return SIS_METHOD_ERROR;
    }
    s_sisdb_client_ask *ask = (s_sisdb_client_ask *)argv_;

    sisdb_client_send_ask(context, ask);

    return SIS_METHOD_OK;    
}
