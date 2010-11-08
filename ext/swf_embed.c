#include <ruby.h>
#ifdef HAVE_RUBY_ST_H
#include <ruby/st.h>
#else
#include <st.h>
#endif
#include <stdint.h>
#include <endian.h>

static void
h16(VALUE str, int value)
{
    uint16_t val = htole16((uint16_t)value);
    char    *chr = (char*)&val;
    rb_str_cat(str, chr, 2);
}

static void
h32(VALUE str, int value)
{
    uint32_t val = htole32((uint32_t)value);
    char    *chr = (char*)&val;
    rb_str_cat(str, chr, 4);
}

static void
h32_repl(char* str, int value)
{
    uint32_t val = htole32((uint32_t)value);
    char    *chr = (char*)&val;
    str[0] = chr[0];
    str[1] = chr[1];
    str[2] = chr[2];
    str[3] = chr[3];
}

static int
sum_i(VALUE key, VALUE value, VALUE *sum)
{
    *sum = INT2FIX(FIX2INT(*sum) + RSTRING_LEN(key) + RSTRING_LEN(value) + 11);
    return ST_CONTINUE;
}

static int
store_i(VALUE key, VALUE value, VALUE str)
{
    rb_str_cat(str, "\x96", 1);
    h16(str, RSTRING_LEN(key) + 2);
    rb_str_cat(str, "\x00", 1);
    rb_str_cat(str, RSTRING_PTR(key), RSTRING_LEN(key));
    rb_str_cat(str, "\x00", 1);

    rb_str_cat(str, "\x96", 1);
    h16(str, RSTRING_LEN(value) + 2);
    rb_str_cat(str, "\x00", 1);
    rb_str_cat(str, RSTRING_PTR(value), RSTRING_LEN(value));
    rb_str_cat(str, "\x00", 1);

    rb_str_cat(str, "\x1D", 1);
    return ST_CONTINUE;
}

static VALUE
create_doaction_tag(VALUE params)
{
    struct buffer_capsule *cap;
    char *buf;
    long  buf_size;
    VALUE str, sum;

    sum = INT2FIX(0);
    rb_hash_foreach(params, sum_i, (st_data_t)&sum);
    buf_size = 2 + 4 + FIX2INT(sum) + 1;
    str = rb_str_buf_new(buf_size);

    rb_str_cat(str, "\x3F\x03", 2);
    h32(str, FIX2INT(sum) + 1);
    rb_hash_foreach(params, store_i, str);
    rb_str_cat(str, "\x00", 1);

    return str;
}

static VALUE
calc_header_size(VALUE swf)
{
    int nbits     = (RSTRING_PTR(swf)[8] >> 3) & 0x1F;
    int rect      = 5 + nbits * 4;
    int rect_size = (rect / 8) + (rect % 8 ? 1 : 0);
    return 3 + 1 + 4 + rect_size + 2 + 2 + 5;
}

static VALUE
swf_embed(VALUE self, VALUE swf, VALUE params)
{
    VALUE result, doaction_tag;
    char* buf;
    long  buf_size, hdr_size, tag_size, swf_size;

    Check_Type(swf,  T_STRING);
    Check_Type(params, T_HASH);

    swf_size = RSTRING_LEN(swf);

    doaction_tag = create_doaction_tag(params);
    tag_size = RSTRING_LEN(doaction_tag);

    buf_size = swf_size + tag_size;
    result = rb_str_buf_new(buf_size);
    buf = RSTRING_PTR(result);

    hdr_size = calc_header_size(swf);
    rb_str_cat(result, RSTRING_PTR(swf), hdr_size);
    h32_repl(RSTRING_PTR(result) + 4, buf_size);

    rb_str_cat(result, RSTRING_PTR(doaction_tag),   tag_size);
    rb_str_cat(result, RSTRING_PTR(swf) + hdr_size, swf_size - hdr_size);

    return result;
}

void Init_swf_embed(void)
{
    VALUE rb_mSwf = rb_define_module("Swf");
    rb_define_singleton_method(rb_mSwf, "embed", swf_embed, 2);
}
