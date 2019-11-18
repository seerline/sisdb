﻿
#include <sis_ai_series.h>
#include <sis_math.h>

///////////////////////////////////////////////////////////////////////////
// 单个因子的参数
///////////////////////////////////////////////////////////////////////////
s_sis_ai_factor_unit *sis_ai_factor_unit_create(const char *fn_)
{
    s_sis_ai_factor_unit *unit = SIS_MALLOC(s_sis_ai_factor_unit, unit);
    sis_strcpy(unit->name, 64, fn_);
    unit->optimals = sis_struct_list_create(sizeof(s_sis_ai_factor_wins));
    unit->weight = 50;
    unit->dispersed = 100;
    return unit;
} 
void sis_ai_factor_unit_destroy(void *unit_)
{
    s_sis_ai_factor_unit *unit = (s_sis_ai_factor_unit *)unit_;
    sis_struct_list_destroy(unit->optimals);
	sis_free(unit);   
}

s_sis_ai_factor *sis_ai_factor_create()
{
    s_sis_ai_factor *o = SIS_MALLOC(s_sis_ai_factor, o);
    o->fields_map = sis_map_int_create();
    o->fields = sis_pointer_list_create();
    o->fields->free = sis_ai_factor_unit_destroy;
    return o;
} 
void sis_ai_factor_destroy(s_sis_ai_factor *cls_)
{
    sis_map_int_destroy(cls_->fields_map);
    sis_pointer_list_destroy(cls_->fields);
    sis_free(cls_);
}
void sis_ai_factor_clear(s_sis_ai_factor *cls_)
{
    sis_map_int_clear(cls_->fields_map);
    sis_pointer_list_clear(cls_->fields);
}

void sis_ai_factor_clone(s_sis_ai_factor *src_, s_sis_ai_factor *des_)
{
    if (!des_ || !src_)
    {
        return ;
    }
    for (int i = 0; i < src_->fields->count; i++)
    {
        s_sis_ai_factor_unit *src = (s_sis_ai_factor_unit *)sis_pointer_list_get(src_->fields, i);
        s_sis_ai_factor_unit *unit = sis_ai_factor_unit_create(src->name);
        unit->index = src->index;
        unit->weight = src->weight;
        unit->dispersed = src->dispersed;
        memmove(&unit->sides,&src->sides, sizeof(s_sis_double_sides));
        sis_struct_list_clone(src->optimals, unit->optimals);
        sis_pointer_list_push(des_->fields, unit);
        sis_map_int_set(des_->fields_map, unit->name, unit->index);
    }   
}
int sis_ai_factor_inc_factor(s_sis_ai_factor *cls_, const char *fn_)
{
    s_sis_ai_factor_unit *unit = sis_ai_factor_unit_create(fn_);
    unit->index = cls_->fields->count;
    sis_pointer_list_push(cls_->fields, unit);
    sis_map_int_set(cls_->fields_map, fn_, unit->index);
    return unit->index;
}
s_sis_ai_factor_unit *sis_ai_factor_get(s_sis_ai_factor *cls_, const char *fn_)
{
    int index = sis_map_int_get(cls_->fields_map, fn_);
    return (s_sis_ai_factor_unit *)sis_pointer_list_get(cls_->fields, index);
}
double sis_ai_factor_get_weight(s_sis_ai_factor *cls_)
{
    double weight = 0.0;
    for (int k = 0; k < cls_->fields->count; k++)
    {
        s_sis_ai_factor_unit *funit = (s_sis_ai_factor_unit *)sis_pointer_list_get(cls_->fields, k);
        weight += funit->weight; 
    }
    return weight;
}

double sis_ai_factor_get_normalization(s_sis_ai_factor *cls_, s_sis_ai_factor_unit *unit_, double in_)
{
    double o = 0.0;
    if (in_ >= unit_->sides.up_minv && in_ <= unit_->sides.up_maxv) 
    {
        // 买单靠最大
        o = (in_ - unit_->sides.up_minv) / (unit_->sides.up_maxv - unit_->sides.up_minv);
    }
    if (in_ >= unit_->sides.dn_minv && in_ <= unit_->sides.dn_maxv)
    {
        // 卖单靠最小
        o = (unit_->sides.dn_maxv - in_) / (unit_->sides.dn_minv - unit_->sides.dn_maxv);
    }
    return o;
}

// 这里求最优命中
void sis_ai_factor_calc_optimals(s_sis_ai_factor_unit *unit_)
{
    int max_gaps = unit_->optimals->count * SIS_SERIES_GAPS;
    int min_gaps = SIS_SERIES_GAPS / 2;

    s_sis_struct_list *ilist = sis_struct_list_create(sizeof(s_sis_optimal_split)); 
    s_sis_double_list *vlist = sis_double_list_create(); 
    for (int i = 0; i < unit_->optimals->count; i++)
    {
        s_sis_ai_factor_wins *wins = (s_sis_ai_factor_wins *)sis_struct_list_get(unit_->optimals, i);
        sis_double_list_set(vlist, wins->sides.up_minv);
        sis_double_list_set(vlist, wins->sides.up_maxv);
        sis_double_list_set(vlist, wins->sides.dn_minv);
        sis_double_list_set(vlist, wins->sides.dn_maxv);
    }
    sis_double_list_simple_split(vlist, max_gaps);
    // 计算命中数
    for (int i = 0; i < vlist->split->count; i++)
    {
        s_sis_double_split *split =(s_sis_double_split *)sis_struct_list_get(vlist->split, i);

        s_sis_optimal_split oks;
        memset(&oks, 0, sizeof(s_sis_optimal_split));
        for (int k = 0; k < unit_->optimals->count; k++)
        {
            s_sis_ai_factor_wins *wins = (s_sis_ai_factor_wins *)sis_struct_list_get(unit_->optimals, k);
            if (sis_is_mixed(split->minv, split->maxv, wins->sides.up_minv, wins->sides.up_maxv))
            {
                oks.ups++;
            }
            if (sis_is_mixed(split->minv, split->maxv, wins->sides.dn_minv, wins->sides.dn_maxv))
            {
                oks.dns++;
            }
        }
        sis_struct_list_push(ilist, &oks);        
        printf("simple_split [%3d] : %7.3f %7.3f | %.2f %.2f\n",i, split->minv, split->maxv, oks.ups, oks.dns);
    }
    // 求最大命中数
    double max_ups = 0;
    double max_dns = 0;
    int max_up_index = -1;
    int max_dn_index = -1;
    for (int i = 0; i < vlist->split->count; i++)
    {
        s_sis_optimal_split *optimal =(s_sis_optimal_split *)sis_struct_list_get(ilist, i);
        int start = sis_max(0, i - min_gaps);
        int stop = sis_min(vlist->split->count, i + min_gaps);
        while(start < stop)
        {
            s_sis_double_split *next =(s_sis_double_split *)sis_struct_list_get(vlist->split, start);
            optimal->sum_ups += optimal->ups;
            optimal->sum_dns += optimal->dns;
            optimal->split.minv = SIS_MINF(optimal->split.minv, next->minv);
            optimal->split.maxv = SIS_MAXF(optimal->split.maxv, next->maxv);
            start++;
        }
        if (max_up_index < 0)
        {
            max_up_index = 0;
            max_ups = optimal->sum_ups;
        }
        else
        {
            if (optimal->sum_ups > max_ups)
            {
                max_up_index = i;
                max_ups = optimal->sum_ups;
            }
        }
        if (max_dn_index < 0)
        {
            max_dn_index = 0;
            max_dns = optimal->sum_dns;
        }
        else
        {
            if (optimal->sum_dns > max_dns)
            {
                max_dn_index = i;
                max_dns = optimal->sum_dns;
            }
        }
        printf("max [%3d : [%d %d] %.2f  %.2f : %7.3f %7.3f |\n",i, max_up_index, max_dn_index, optimal->sum_ups, optimal->sum_dns, optimal->split.minv, optimal->split.maxv);
    }
    // 寻找附近的取值，如果一样就扩大范围
    for (int i = max_up_index + 1; i < vlist->split->count; i++)
    {
        s_sis_optimal_split *optimal =(s_sis_optimal_split *)sis_struct_list_get(ilist, i);
        if (optimal->sum_ups < max_ups)
        {
            break;
        }
        s_sis_optimal_split *ago =(s_sis_optimal_split *)sis_struct_list_get(ilist, max_up_index);
        max_up_index = i;
        optimal->split.minv = SIS_MINF(optimal->split.minv, ago->split.minv);
        optimal->split.maxv = SIS_MAXF(optimal->split.maxv, ago->split.maxv);                
    }
    for (int i = max_dn_index + 1; i < vlist->split->count; i++)
    {
        s_sis_optimal_split *optimal =(s_sis_optimal_split *)sis_struct_list_get(ilist, i);
        if (optimal->sum_dns < max_dns)
        {
            break;
        }
        s_sis_optimal_split *ago =(s_sis_optimal_split *)sis_struct_list_get(ilist, max_dn_index);
        max_dn_index = i;
        optimal->split.minv = SIS_MINF(optimal->split.minv, ago->split.minv);
        optimal->split.maxv = SIS_MAXF(optimal->split.maxv, ago->split.maxv);                
    }
    s_sis_optimal_split *optimal_up =(s_sis_optimal_split *)sis_struct_list_get(ilist, max_up_index);
    s_sis_optimal_split *optimal_dn =(s_sis_optimal_split *)sis_struct_list_get(ilist, max_dn_index);

    if (sis_is_mixed(optimal_up->split.minv, optimal_up->split.maxv, optimal_dn->split.minv, optimal_dn->split.maxv))
    {
        // 如果两个区域有重叠，就去重
        if (max_up_index > max_dn_index)
        {
            double mid = (optimal_dn->split.maxv - optimal_up->split.minv) / 2.0;
            optimal_up->split.minv += mid;
            optimal_dn->split.maxv += optimal_up->split.minv - 0.001;
        }
        else
        {
            double mid = (optimal_up->split.maxv - optimal_dn->split.minv) / 2.0;
            optimal_dn->split.minv += mid;
            optimal_up->split.maxv += optimal_dn->split.minv - 0.001;
        } 
    }
    unit_->sides.up_minv =  optimal_up->split.minv;
    unit_->sides.up_maxv =  optimal_up->split.maxv;
    unit_->sides.dn_minv =  optimal_dn->split.minv;
    unit_->sides.dn_maxv =  optimal_dn->split.maxv;

    printf(":up: [%d %.2f] : %7.3f %7.3f |\n",max_up_index, optimal_up->sum_ups, 
        optimal_up->split.minv, optimal_up->split.maxv);
    printf(":dn: [%d %.2f] : %7.3f %7.3f |\n",max_dn_index,  optimal_dn->sum_dns, 
        optimal_dn->split.minv, optimal_dn->split.maxv);

    // 求离散度
    double dispersed = 0.001;
    if (max_up_index > max_dn_index)
    {
        dispersed = (optimal_up->split.minv - optimal_dn->split.maxv) / (vlist->maxv - vlist->minv);
        // (optimal_up->split.maxv - optimal_dn->split.minv);
    }
    else
    {
        dispersed = (optimal_dn->split.minv - optimal_up->split.maxv) / (vlist->maxv - vlist->minv);
        // (optimal_dn->split.maxv - optimal_up->split.minv);
    } 
    unit_->dispersed = dispersed * 100.0; // 1882 / 6839
    printf("dispersed = %.2f\n", unit_->dispersed);

    sis_struct_list_destroy(ilist);   
    sis_double_list_destroy(vlist);
}
///////////////////////////////////////////////////////////////////////////
// 单个样本的数据(通常指一天的数据)
///////////////////////////////////////////////////////////////////////////

s_sis_ai_series_unit *sis_ai_series_unit_create()
{
    s_sis_ai_series_unit *o = SIS_MALLOC(s_sis_ai_series_unit, o);
    o->klists = sis_struct_list_create(sizeof(s_sis_ai_series_unit_key));
    o->vlists = sis_pointer_list_create();
    o->vlists->free = sis_free_call;

    o->cur_factors = NULL;
    o->out_factors = NULL;
    return o;
}
void sis_ai_series_unit_clear_factors(s_sis_ai_series_unit *cls_)
{
    if (cls_->cur_factors)
    {
        sis_ai_factor_destroy(cls_->cur_factors);
        cls_->cur_factors = NULL;
    }
    if (cls_->out_factors)
    {
        sis_ai_factor_destroy(cls_->out_factors);
        cls_->out_factors = NULL;
    }
}
void sis_ai_series_unit_destroy(void *cls_)
{
    s_sis_ai_series_unit *cls = (s_sis_ai_series_unit *)cls_;

    sis_struct_list_destroy(cls->klists);
    sis_pointer_list_destroy(cls->vlists);

    sis_ai_series_unit_clear_factors(cls);

    sis_free(cls);
}
void sis_ai_series_unit_clear(s_sis_ai_series_unit *cls_)
{
    sis_struct_list_clear(cls_->klists);
    sis_pointer_list_clear(cls_->vlists);

    sis_ai_series_unit_clear_factors(cls_);
}

void sis_ai_series_unit_init_factors(s_sis_ai_series_unit *cls_, s_sis_ai_factor *factors_)
{
    sis_ai_series_unit_clear_factors(cls_);
    if (factors_)
    {
        cls_->cur_factors = sis_ai_factor_create();
        sis_ai_factor_clone(factors_, cls_->cur_factors);
    }
}

int sis_ai_series_unit_inc_key(s_sis_ai_series_unit *cls_,  int sno_, double newp_)
{
    if (!cls_->cur_factors)
    {
        return -1;
    }
    s_sis_ai_series_unit_key key;
    key.sno = sno_;
    key.newp = newp_;

    int fnums = cls_->cur_factors->fields->count;
    double *val = sis_malloc(sizeof(double) * fnums);
    memset(val, 0, sizeof(double) * fnums);
    sis_pointer_list_push(cls_->vlists, val);
    sis_struct_list_push(cls_->klists, &key);
    return cls_->klists->count - 1;
}

// 根据时间插入对应的数据, 需要补齐缺失的数据
int sis_ai_series_unit_set_val(s_sis_ai_series_unit *cls_, int index_, const char *fn_, double value_)
{
    if (!cls_->cur_factors)
    {
        return -1;
    }
    if (index_ < 0 || index_ > cls_->vlists->count - 1)
    {
        return -2;
    }
    s_sis_ai_factor_unit *factor = sis_ai_factor_get(cls_->cur_factors, fn_);
    if (!factor)
    {
        return -3;
    }
    double *val = (double *)sis_pointer_list_get(cls_->vlists, index_);

    val[factor->index] = value_;
    
    return 0;
}

s_sis_ai_series_unit_key *sis_ai_series_unit_get_key(s_sis_ai_series_unit *cls_, int index_)
{
    return sis_struct_list_get(cls_->klists, index_);
}

double sis_ai_series_unit_get(s_sis_ai_series_unit *cls_, int fidx_, int kidx_)
{
    if (!cls_->cur_factors)
    {
        return -1;
    }
    double *val = (double *)sis_pointer_list_get(cls_->vlists, kidx_);
    if (val && fidx_ >= 0 && fidx_ < cls_->cur_factors->fields->count)
    {
        return val[fidx_];
    }
    return 0.0;
}

int sis_ai_series_unit_get_field_size(s_sis_ai_series_unit *cls_)
{
    if (!cls_->cur_factors)
    {
        return -1;
    }
    return cls_->cur_factors->fields->count;
}
int sis_ai_series_unit_get_size(s_sis_ai_series_unit *cls_)
{
    return cls_->klists->count;
}

///////////////////////////////////////////////////////////////////////////
//  多个样本的管理类
//////////////////////////////////////////////////////////////////////////

s_sis_ai_series_class *sis_ai_series_class_create()
{
    s_sis_ai_series_class *o = SIS_MALLOC(s_sis_ai_series_class, o);
    o->factors = sis_ai_factor_create();
    o->studys = sis_pointer_list_create();
    o->studys->free = sis_ai_series_unit_destroy;
    // o->checks = sis_pointer_list_create();
    // o->checks->free = sis_ai_series_unit_destroy;
    // o->ranges = sis_struct_list_create(sizeof(s_sis_ai_series_range));
    return o;
}
void sis_ai_series_class_destroy(s_sis_ai_series_class *cls_)
{
    sis_ai_factor_destroy(cls_->factors);
    sis_pointer_list_destroy(cls_->studys);
    // sis_pointer_list_destroy(cls_->checks);
    // sis_struct_list_destroy(cls_->ranges);
}
void sis_ai_series_class_clear(s_sis_ai_series_class *cls_)
{
    // 恢复参数为原始值
    sis_ai_factor_clear(cls_->factors);
    sis_pointer_list_clear(cls_->studys);
    // sis_pointer_list_clear(cls_->checks);
    // sis_struct_list_clear(cls_->ranges);
}

int sis_ai_series_class_inc_factor(s_sis_ai_series_class *cls_, const char *fn_)
{
    return sis_ai_factor_inc_factor(cls_->factors, fn_);
}
s_sis_ai_series_unit *sis_ai_series_class_get_study(s_sis_ai_series_class *cls_, int mark_)
{
    for (int i = cls_->studys->count - 1; i >= 0; i--)
    {
        s_sis_ai_series_unit *unit = (s_sis_ai_series_unit *)sis_pointer_list_get(cls_->studys, i);
        if (unit->mark == mark_)
        {
            return unit;
        }
    }
    // 没有找到就新建，新建时会传递当前的因子库，
    // 注意：即使这里赋值，也需要在计算当日数据后，更新备份的特征值
    s_sis_ai_series_unit *unit = sis_ai_series_unit_create();
    unit->mark = mark_;
    sis_pointer_list_push(cls_->studys, unit);
    sis_ai_series_unit_init_factors(unit, cls_->factors);
    return unit;    
}
// ---------  这里是求出最优收益的处理函数 ---------- // 
// 平仓

void _make_order_ask_close(s_sis_ai_order_census *census_, s_sis_ai_order *ago,  s_sis_ai_order *move)
{
	double money = move->newp * SIS_AI_MIN_VOLS;
    double income = (ago->newp * (1 - SIS_AI_MIN_COST) - move->newp);
	if (income > 0.001)
	{
		census_->bid_oks++;
	}
	census_->bid_income += income * SIS_AI_MIN_VOLS;
	census_->sum_money += money;
	census_->cur_bidv -= SIS_AI_MIN_VOLS;
	census_->use_money -= money;
    // census_->cur_bidnums--;
} 
void _make_order_bid_close(s_sis_ai_order_census *census_, s_sis_ai_order *ago, s_sis_ai_order *move)
{
	double money = move->newp * SIS_AI_MIN_VOLS;
    double income = (move->newp - ago->newp * (1 + SIS_AI_MIN_COST));
	if (income > 0.001)
	{
		census_->ask_oks++;
	}
	census_->ask_income += income * SIS_AI_MIN_VOLS;
	census_->sum_money += money;
	census_->cur_askv -= SIS_AI_MIN_VOLS;
	census_->bid_money += money;
	census_->use_money += money;
    // census_->cur_asknums--;
} 
// 开仓
void _make_order_ask_open(s_sis_ai_order_census *census_, s_sis_ai_order *move)
{
    // census_->cur_asknums++;
	census_->asks++;
	census_->cur_avgp = (census_->cur_avgp * census_->cur_askv + move->newp * move->vols)/(census_->cur_askv + move->vols);
	census_->cur_askv += move->vols;
	double money = move->newp * move->vols;
	census_->sum_money += money;
	// 
	census_->use_money -= money;
	census_->ask_money = sis_max(census_->ask_money, fabs(census_->use_money));
} 
void _make_order_bid_open(s_sis_ai_order_census *census_, s_sis_ai_order *move)
{
    // census_->cur_bidnums++;
	census_->bids++;
	census_->cur_avgp = (census_->cur_avgp * census_->cur_bidv + move->newp * move->vols)/(census_->cur_bidv + move->vols);
	census_->cur_bidv += move->vols;
	double money = move->newp * move->vols;
	census_->sum_money += money;
	// 只要卖开仓就增加持仓资金
	census_->bid_money += money;
	census_->use_money += money;  // 增加可用资金
} 

void _make_order(s_sis_ai_series_unit *unit_, s_sis_struct_list *orders, s_sis_ai_order_census *census_)
{
	s_sis_ai_order order;
    for (int i = 0; i < unit_->vlists->count; i++)
    {
        s_sis_ai_series_unit_key *move = sis_ai_series_unit_get_key(unit_, i);
		if (move->cmd & SIS_SERIES_OUT_ASK1 || move->cmd == SIS_SERIES_OUT_BID1)
		{
			order.cmd = move->cmd;
			order.newp = move->newp;
            order.vols = SIS_AI_MIN_VOLS;
			sis_struct_list_push(orders, &order);
		}
		// if (move->cmd == SIS_SERIES_OUT_ASK2 || move->cmd == SIS_SERIES_OUT_BID2)
		// {
		// 	order.cmd = move->cmd;
		// 	order.newp = move->newp;
        //     order.vols = 200;
		// 	sis_struct_list_push(orders, &order);
		// }
    }
	// 统计信息
	memset(census_, 0, sizeof(s_sis_ai_order_census));
	for (int i = 0; i < orders->count; i++)
	{
		s_sis_ai_order *move = (s_sis_ai_order *)sis_struct_list_get(orders, i);
		if (move->cmd & SIS_SERIES_OUT_ASK)
		{
			if (census_->cur_bidv > 0)
			{
				int count = (int)(census_->cur_bidv / 100);
				for (int i = 0; i < count; i++)
				{
					s_sis_ai_order *ago = (s_sis_ai_order *)sis_struct_list_offset(orders, move, -1 - i);
					_make_order_ask_close(census_, ago, move);
				}
			}
			if (i < orders->count - 1)
			{
				_make_order_ask_open(census_, move);
			}
			else
			{
				// 最后一条记录，平多单
				int count = (int)(census_->cur_askv / 100);
				for (int i = 0; i < count; i++)
				{
					s_sis_ai_order *ago = (s_sis_ai_order *)sis_struct_list_offset(orders, move, -1 - i);
					_make_order_bid_close(census_, ago, move);
				}
			}
		}
		if (move->cmd & SIS_SERIES_OUT_BID)
		{
			if (census_->cur_askv > 0)
			{
				int count = (int)(census_->cur_askv / 100);
				for (int i = 0; i < count; i++)
				{
					s_sis_ai_order *ago = (s_sis_ai_order *)sis_struct_list_offset(orders, move, -1 - i);
					_make_order_bid_close(census_, ago, move);
				}
			}
			if (i < orders->count - 1)
			{
				_make_order_bid_open(census_, move);
			}
			else
			{
				// 最后一条记录，平空单
				int count = (int)(census_->cur_bidv / 100);
				for (int i = 0; i < count; i++)
				{
					s_sis_ai_order *ago = (s_sis_ai_order *)sis_struct_list_offset(orders, move, -1 - i);
					_make_order_ask_close(census_, ago, move);
				}			
			}
		}
		// printf("[%d] %7.2f [%4d : %4d %7.2f] direct = %d | %7.2f %7.2f\n", i, move->newp, census_->cur_askv, census_->cur_bidv, census_->cur_avgp, move->cmd, census_->ask_income, census_->bid_income);
	}
	census_->ask_rate = census_->ask_money ? (census_->ask_income) / (census_->ask_money) * 100.0 : 0.0;
	census_->bid_rate = census_->bid_money ? (census_->bid_income) / (census_->bid_money) * 100.0 : 0.0;
	census_->sum_incr = (census_->ask_income + census_->bid_income) / (census_->ask_money + census_->bid_money) * 100.0;
	census_->sum_winr = (double)(census_->ask_oks + census_->bid_oks + 1) /(double)(census_->asks + census_->bids + 2);
	census_->sum_inwinr = census_->sum_incr * census_->sum_winr;
}

s_sis_ai_series_unit *sis_ai_series_class_new_check(int mark_, s_sis_ai_factor *factor_)
{
    // 注意：即使这里赋值，也需要在计算当日数据后，更新备份的特征值
    s_sis_ai_series_unit *unit = sis_ai_series_unit_create();
    unit->mark = mark_;
    sis_ai_series_unit_init_factors(unit, factor_);
    return unit;    
}
int sis_ai_series_class_calc_check(s_sis_ai_series_unit *unit_, s_sis_ai_order_census *census_)
{
    // 以前一个为基数
    s_sis_ai_factor *factor = (s_sis_ai_factor *)unit_->cur_factors;
    // int count = sis_ai_series_unit_get_size(unit_);

    double weight = sis_ai_factor_get_weight(factor);
    for (int i = 0; i < unit_->vlists->count; i++)
    {
        s_sis_ai_series_unit_key *key = sis_ai_series_unit_get_key(unit_, i);
        key->cmd = SIS_SERIES_OUT_NONE;
        double cmd = 0.0;
        for (int k = 0; k < factor->fields->count; k++)
        {
            s_sis_ai_factor_unit *funit = (s_sis_ai_factor_unit *)sis_pointer_list_get(factor->fields, k);
            double val = sis_ai_factor_get_normalization(factor, funit, sis_ai_series_unit_get(unit_, k, i));
            cmd += val * funit->weight; 
            // printf(" %s : [%.0f] %f %f|", funit->name, funit->weight, val, cmd);
        }
        cmd = cmd / weight;
        printf("%d : %f : %f:", key->sno, key->newp, cmd);
        // 得到最终的买卖信号
        if (cmd > 0.01)
        {
            key->cmd = SIS_SERIES_OUT_ASK1;
        }
        if (cmd < -0.01)
        {
            key->cmd = SIS_SERIES_OUT_BID1;
        }
    }
    s_sis_struct_list *orders = sis_struct_list_create(sizeof(s_sis_ai_order));
	_make_order(unit_, orders, census_);
    sis_struct_list_destroy(orders);
	return 0;
}
// 单因子，原始值学习
static int _calc_optimal_single(s_sis_ai_series_unit *unit_, 
    int fidx_, s_sis_double_sides *sides_, 
    s_sis_ai_order_census *census_)
{
	// 生成委托指令
    for (int i = 0; i < unit_->vlists->count; i++)
    {
        s_sis_ai_series_unit_key *key = sis_ai_series_unit_get_key(unit_, i);
        key->cmd = SIS_SERIES_OUT_NONE;
        double value = sis_ai_series_unit_get(unit_, fidx_, i);

        if (value > sides_->up_minv && value < sides_->up_maxv)
        {
            key->cmd = SIS_SERIES_OUT_ASK1;
        }
        if (value > sides_->dn_minv && value < sides_->dn_maxv)
        {
            key->cmd = SIS_SERIES_OUT_BID1;
        }
    }
    s_sis_struct_list *orders = sis_struct_list_create(sizeof(s_sis_ai_order));
	_make_order(unit_, orders, census_);
    sis_struct_list_destroy(orders);
	return 0;
}
// 求某日的指标独立的最优参数，需时较长
void sis_ai_factor_optimal_start(s_sis_ai_series_unit *unit_)
{
    if (!unit_->cur_factors)
    {
        return ;
    }
    // 仅仅在做 最优计算时才会生成 out_factors 
    if (unit_->out_factors)
    {
        sis_ai_factor_destroy(unit_->out_factors);
    }
    unit_->out_factors = sis_ai_factor_create();
    sis_ai_factor_clone(unit_->cur_factors, unit_->out_factors);
    
	s_sis_double_list *vlist = sis_double_list_create(); 
    s_sis_struct_list *optimals = sis_struct_list_create(sizeof(s_sis_ai_factor_wins));
    for (int k = 0; k < unit_->out_factors->fields->count; k++)
    {
        s_sis_ai_factor_unit *factor = (s_sis_ai_factor_unit *)sis_pointer_list_get(unit_->out_factors->fields, k);
        sis_double_list_clear(vlist);
        for (int i = 0; i < sis_ai_series_unit_get_size(unit_); i++)
        {
            double val = sis_ai_series_unit_get(unit_, k, i);
            sis_double_list_set(vlist, val);   
        }
        if (!sis_double_list_count_sides(vlist, SIS_SERIES_GAPS))
        {
            continue;
        }
        double max_inwinr = 0.001;
        s_sis_ai_order_census  cur_census;
        sis_struct_list_clear(optimals);
        for (int i = 0; i < vlist->sides->count; i++)
        {
            s_sis_double_sides *sides =(s_sis_double_sides *)sis_struct_list_get(vlist->sides, i);
            // 这里计算单个因子的收益
        	_calc_optimal_single(unit_, factor->index, sides, &cur_census);
            if (cur_census.sum_inwinr > 0.001)
            {
                s_sis_ai_factor_wins wins;
                memset(&wins, 0, sizeof(s_sis_ai_factor_wins));
                memmove(&wins.sides, sides, sizeof(s_sis_double_sides));
                wins.incr = cur_census.sum_incr;
                wins.winr = cur_census.sum_winr;
                wins.inwinr = cur_census.sum_inwinr;
                max_inwinr = SIS_MAXF(max_inwinr, wins.inwinr);
                sis_struct_list_push(optimals, &wins);
            }
        	printf("-[%d]- ask: %.2f %.2f = %d, %.2f %.0f %.4f || bid: %.2f %.2f =  %d, %.2f %.0f %.4f || %.2f rate = %.4f ok = %.2f inok = %.2f\n", 
        		i, sides->up_minv, sides->up_maxv, 
        			cur_census.asks, cur_census.ask_income, cur_census.ask_money, cur_census.ask_rate,
        		sides->dn_minv, sides->dn_maxv, 
        			cur_census.bids, cur_census.bid_income, cur_census.bid_money, cur_census.bid_rate,
        		cur_census.sum_money, cur_census.sum_incr, cur_census.sum_winr, cur_census.sum_inwinr);
        }
        // 把相对合格的数据存到 optimals 中
        int count = 0;
        for (int i = 0; i < optimals->count; i++)
        {
            s_sis_ai_factor_wins *win = (s_sis_ai_factor_wins *)sis_struct_list_get(optimals, i);
            // 只保存相对收益较大的，并且不超过5个
            if (win->inwinr > max_inwinr * 0.1 && count < SIS_SERIES_TOPS)
            {
                count++;
                sis_struct_list_push(factor->optimals, win);
                printf(":[%d]: %.4f | %.4f %.4f | ask: %.2f %.2f bid: %.2f %.2f \n", 
                    factor->optimals->count, win->inwinr, win->incr, win->winr, 
                    win->sides.up_minv, win->sides.up_maxv, 
                    win->sides.dn_minv, win->sides.dn_maxv);
            }
        }  
    }
    sis_struct_list_destroy(optimals);   
    sis_double_list_destroy(vlist);
}

void sis_ai_factor_optimal_stop(s_sis_ai_series_unit *unit_)
{
    if (!unit_->out_factors)
    {
        return ;
    }
    // 对每个因子进行最优化计算 得到最佳买卖区间 和 离散度
    for (int k = 0; k < unit_->out_factors->fields->count; k++)
    {
        s_sis_ai_factor_unit *factor = (s_sis_ai_factor_unit *)sis_pointer_list_get(unit_->out_factors->fields, k);
        sis_ai_factor_calc_optimals(factor);
    }
    // 对权重进行分片，回测之前所有数据，以求得最大收益时权重的分配比例，作为下一天的实际权重
    // 这里计算量较大，因子个数 * 分片个数
    // 对全部因子的权重根据当日数据进行再次调优
    
}
// ---------  这里开始处理数据 ---------- // 
s_sis_ai_factor *sis_ai_series_class_study(s_sis_ai_series_class *cls_)
{
    s_sis_ai_series_unit *ago = NULL;
    for (int i = 0; i < cls_->studys->count; i++)
    {
        s_sis_ai_series_unit *unit = (s_sis_ai_series_unit *)sis_pointer_list_get(cls_->studys, i);
        if (!ago)
        {
            sis_ai_series_unit_init_factors(unit, cls_->factors);
        }
        else
        {
            sis_ai_series_unit_init_factors(unit, ago->out_factors);
        }        
        sis_ai_factor_optimal_start(unit);
        sis_ai_factor_optimal_stop(unit);
        ago = unit;
    }    
    // 全部日期处理完，
    // 由于开始和结束的权重很有可能有很大偏差，需要对偏差值进行最优化处理
    for (int i = 0; i < cls_->studys->count; i++)
    {
        s_sis_ai_series_unit *unit = (s_sis_ai_series_unit *)sis_pointer_list_get(cls_->studys, i);
        for (int k = 0; k < unit->cur_factors->fields->count; k++)
        {
            s_sis_ai_factor_unit *factor =(s_sis_ai_factor_unit *)sis_struct_list_get(unit->cur_factors->fields, k);
            printf("[%d] %s weight = %f, dispersed = %f [%f %f] [%f %f]\n", 
                unit->mark, factor->name, factor->weight, factor->dispersed,
                factor->sides.up_minv,factor->sides.up_maxv,
                factor->sides.dn_minv,factor->sides.dn_maxv);
        }
    }    

    return ago->out_factors;
}
#if 0

static struct s_sis_ai_factor_wins one[] = {
    // { 0.5, 70.0, 35.0, {1.5, 3.5, -2.5, -0.1}},
    // { 0.5, 70.0, 35.0, {1.5, 2.5, -1.5, -0.1}},
    // { 0.5, 70.0, 35.0, {1.0, 2.5, -3.5, -1.5}},
    { 0.5, 70.0, 35.0, {3.5, 4.5, -4.0, -2.0}},
    { 0.5, 70.0, 35.0, {3.0, 3.5, -3.0, -1.5}},
    { 0.5, 70.0, 35.0, {0.5, 2.5, -1.5,  1.5}},
    { 0.5, 70.0, 35.0, {1.0, 3.5, -0.5,  2.5}},
};

int main()
{
    s_sis_ai_factor_unit *unit = sis_ai_factor_unit_create("test");
    for (int i = 0; i < sizeof(one)/sizeof(s_sis_ai_factor_wins); i++)
    {
        sis_struct_list_push(unit->optimals, &one[i]);
    }
    sis_ai_factor_calc_optimals(unit);
    sis_ai_factor_unit_destroy(unit);
}
#endif
#if 0
max [  9 : [0 9] 0  40 :  -3.317  -1.608 |
max [ 10 : [0 9] 0  40 :  -3.146  -1.437 |
max [ 11 : [0 9] 0  40 :  -2.975  -1.266 |
max [ 35 : [35 9] 40  0 :   1.129   2.838 |
max [ 36 : [35 9] 40  0 :   1.300   3.009 |
max [ 37 : [35 9] 40  0 :   1.471   3.180 |

typedef struct _water
{
    int    sno;
    double newp;
    double factor[3];
} _water;

static struct _water one[] = {
    { 10, 20.50, {0.597,  0.060, 5800.0}},
    { 20, 20.70, {0.697,  0.260, 5000.0}},
    { 30, 21.20, {0.797,  0.660, 9000.0}},
    { 40, 21.50, {0.897,  0.560, 6000.0}},
    { 50, 21.30, {0.697,  0.260, 5500.0}},
    { 60, 20.90, {0.500, -0.160, 1000.0}},
    { 70, 20.60, {0.397, -0.360, 4500.0}},
    { 80, 20.30, {0.197, -0.560, 6300.0}},
    { 90, 20.10, {0.297, -0.760, 6600.0}},
};

static struct _water two[] = {
    { 10, 20.51, {0.597,  0.060, 3000.0}},
    { 20, 20.71, {0.697,  0.260, 8000.0}},
    { 30, 21.21, {0.797,  0.660, 9000.0}},
    { 40, 21.51, {0.897,  0.560, 7000.0}},
    { 50, 21.31, {0.697,  0.260, 5000.0}},
    { 60, 20.91, {0.500, -0.160, 1000.0}},
    { 70, 20.61, {0.397, -0.360, 2000.0}},
    { 80, 20.31, {0.197, -0.560, 3000.0}},
    { 90, 20.11, {0.297, -0.760, 6000.0}},
};

int main()
{
    s_sis_ai_series_class *list = sis_ai_series_class_create();

    sis_ai_series_class_inc_factor(list, "m1");
    sis_ai_series_class_inc_factor(list, "m2");
    sis_ai_series_class_inc_factor(list, "m3");

    {
        s_sis_ai_series_unit *unit = sis_ai_series_class_get_study(list, 1001);
        for (int i = 0; i < 9; i++)
        {
            int index = sis_ai_series_unit_inc_key(unit, one[i].sno, one[i].newp);
            sis_ai_series_unit_set_val(unit, index, "m1", one[i].factor[0]);
            sis_ai_series_unit_set_val(unit, index, "m2", one[i].factor[1]);
            sis_ai_series_unit_set_val(unit, index, "m3", one[i].factor[2]);
        }
    }
    {
        s_sis_ai_series_unit *unit = sis_ai_series_class_get_study(list, 1002);
        for (int i = 0; i < 9; i++)
        {
            int index = sis_ai_series_unit_inc_key(unit,  two[i].sno, two[i].newp);
            sis_ai_series_unit_set_val(unit, index, "m1", two[i].factor[0]);
            sis_ai_series_unit_set_val(unit, index, "m2", two[i].factor[1]);
            sis_ai_series_unit_set_val(unit, index, "m3", two[i].factor[2]);
        }
    }
    
    for (int m = 1001; m < 1003; m++)
    {
        s_sis_ai_series_unit *unit = sis_ai_series_class_get_study(list, m);
        printf("%d : %d = %d \n", m, list->studys->count, unit->mark);
        for (int i = 0; i < unit->vlists->count; i++)
        {
            s_sis_ai_series_unit_key *key = sis_ai_series_unit_get_key(unit, i);
            printf("%d : %f :", key->sno, key->newp);
            for (int k = 0; k < list->factors->fields->count; k++)
            {
                s_sis_ai_factor_unit *factor = sis_pointer_list_get(list->factors->fields, k);
                printf(" %s : [%.0f] %f |", factor->name, factor->weight, sis_ai_series_unit_get(unit, k, i));
            }
            printf("\n");
        }
    }
    s_sis_double_list *vlist = sis_double_list_create(); 

    for (int i = 0; i < 9; i++)
    {
        sis_double_list_set(vlist,one[i].factor[2]);
    }
    printf(" %d : %f  %f <-> %f\n", vlist->value->count, vlist->avgv, vlist->minv, vlist->maxv);
    s_sis_struct_list *splits = sis_double_list_count_split(vlist, 3);
    for (int i = 0; i < splits->count; i++)
    {
        s_sis_double_split *split = (s_sis_double_split *)sis_struct_list_get(splits, i);
        printf(" %d : %f <-> %f\n", i, split->minv, split->maxv);
        /* code */
    }
 
    sis_double_list_destroy(vlist);

    // sis_ai_series_class_study(list);

    sis_ai_series_class_destroy(list);
    return 0;
}

#endif