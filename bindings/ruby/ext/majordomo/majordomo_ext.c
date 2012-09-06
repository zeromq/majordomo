#include "majordomo_ext.h"

VALUE rb_mMajordomo;

#ifdef HAVE_RUBY_ENCODING_H
rb_encoding *binary_encoding;
#endif

void Init_majordomo_ext()
{
    rb_mMajordomo = rb_define_module("Majordomo");

#ifdef HAVE_RUBY_ENCODING_H
    binary_encoding = rb_enc_find("binary");
#endif

    _init_majordomo_client();
    _init_majordomo_worker();
}