#ifndef MAJORDOMO_RUBINIUS_H
#define MAJORDOMO_RUBINIUS_H

#define RSTRING_NOT_MODIFIED

#ifdef HAVE_RUBY_ENCODING_H
#include <ruby/encoding.h>
extern rb_encoding *binary_encoding;
#define MajordomorEncode(str) rb_enc_associate(str, binary_encoding)
#else
#define MajordomoEncode(str) str
#endif

#define TRAP_BEG
#define TRAP_END

#define FIXNUM_FLAG 0x01

#endif