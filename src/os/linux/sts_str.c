#include <sts_str.h>

int sts_strcpy(char *out_, size_t olen_, const char *in_)
{
	if (!in_ || !out_)
	{
		return 0;
	}
	size_t len = strlen(in_);
	olen_--; //防止字符串最后一位可能是边界
	len = len > olen_ ? olen_ : len;
	memmove(out_, in_, len);
	memset(out_ + len, 0, olen_ - len + 1);
	return (int)len;
}

int sts_strncpy(char *out_, size_t olen_, const char *in_, size_t ilen_)
{
	if (!in_ || !out_)
	{
		return 0;
	}

	size_t len = strlen(in_);
	len = len > ilen_ ? ilen_ : len;
	olen_--; //防止字符串最后一位可能是边界
	len = len > olen_ ? olen_ : len;
	memmove(out_, in_, len);
	memset(out_ + len, 0, olen_ - len + 1);
	return (int)len;
}


// 以第一个字符串为长度，进行比较
int sts_strcase_match(const char *son_, const char *source_)
{
	if (!son_)
	{
		return (son_ == source_) ? 0 : 1;
	}
	if (!source_)
	{
		return 1;
	}
	for (; tolower(*son_) == tolower(*source_); ++son_, ++source_)
	{
		if (*son_ == 0 || *source_ == 0)
		{
			return 0;
		}
	}
	if (*son_ == 0)
	{
		return 0;
	}
	return 1;
}
int sts_strcasecmp(const char *s1_, const char *s2_)
{
	if (!s1_)
	{
		return (s1_ == s2_) ? 0 : 1;
	}
	if (!s2_)
	{
		return 1;
	}
	for (; tolower(*s1_) == tolower(*s2_); ++s1_, ++s2_)
	{
		if (*s1_ == 0 || *s2_ == 0)
		{
			return 0;
		}
	}
	return 1;
	//tolower(*(const unsigned char *)s1_) - tolower(*(const unsigned char *)s2_);
}
char *sts_strdup(const char *str_, size_t len_) //STS_MALLOC
{
	if (!str_) { return NULL; }
	size_t len = len_;
	if (len_ == 0)
	{
		len = strlen(str_);
	}
	char *buffer = (char *)sts_malloc(len + 1);
	memcpy(buffer, str_, len);
	buffer[len] = 0;
	return buffer;
}

char *sts_str_sprintf(size_t mlen_, const char *fmt_, ...) 
{
    char *str = (char *)sts_malloc(mlen_ + 1);
  	va_list args;
    va_start(args, fmt_);
	// snprintf(str, mlen_, fmt_, args);
	vsnprintf(str, mlen_, fmt_, args);
    va_end(args);
    return str;
}

const char *sts_str_split(const char *s, size_t *len_, char c)
{
	*len_ = 0;
	const char *ptr = s;
	while (ptr && *ptr && (unsigned char)*ptr != c)
	{
		ptr++;
		*len_ = *len_ + 1;
	}
	return ptr;
}
void sts_str_substr(char *out_, size_t olen_, const char *in_, char c, int idx_)
{
    int count = 0;
    const char *ptr = in_;
    const char *start = ptr;
    while(ptr&&*ptr)
    {
        if (*ptr == c) {           
            if(idx_ == count){
               	sts_strncpy(out_, olen_, start, ptr - start);
				return; 
            }
            ptr++;
			start = ptr;
			count++;            
        } else {
            ptr++;
        }
    }
    if (ptr > start) {
        sts_strncpy(out_, olen_, start, ptr - start);
    } else {
        sts_strcpy(out_, olen_, in_);
    }
}

// int sts_strlen_right(const char *str_,const char * right_,const char *ctf_)
// {
// 	int len=0;
// 	while(str_&&*str_&&!strchr(ctf_, (unsigned char)*str_)&&str_<right_) {
// 	// while(str_&&*str_&&!strchr(ctf_, (unsigned char)*str_)) {	
// 		printf("%x\n",(unsigned char)*str_);
// 		len++;
// 		str_++;
// 	}
// 	return len;
// }

// int sts_strlen_left(const char *str_,const char * left_,const char *ctf_)
// {
// 	int len=0;
// 	// while(str_&&*str_&&!strchr(ctf_, (unsigned char)*str_&&str_>left_)) {
// 	while(str_&&*str_&&!strchr(ctf_, (unsigned char)*str_)) {
// 		printf("%x\n",(unsigned char)*str_);
// 		len++;
// 		str_--;
// 	}
// 	return --len;
// }
const char *sts_str_getline(const char *e_, int *len_, const char *s_, size_t size_)
{
	if (!e_||!s_) return NULL;
	while(*e_&&e_>s_&&!strchr("\r\n", (unsigned char)*e_)) {
		e_--;
	}
	e_++; // 把回车跳过
	// printf("%.30s\n",e_);
	*len_ = 0;
	int slen = size_- (e_ - s_);
	const char *p = e_;
	while(*p&&*len_<slen&&!strchr("\r\n", (unsigned char)*p)) {
		p++;
		*len_=*len_+1;
	}
	// printf("%.30s   %d\n",e_, *len_);

	return e_;
}
