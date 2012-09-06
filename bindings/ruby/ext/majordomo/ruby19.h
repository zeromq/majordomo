#ifndef MAJORDOMO_RUBY19_H
#define MAJORDOMO_RUBY19_H

#include <ruby/encoding.h>
#include <ruby/io.h>

extern rb_encoding *binary_encoding;
#define MajordomoEncode(str) rb_enc_associate(str, binary_encoding)

#define TRAP_BEG
#define TRAP_END

#endif