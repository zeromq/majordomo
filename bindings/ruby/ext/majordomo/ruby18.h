#ifndef MAJORDOMO_RUBY18_H
#define MAJORDOMO_RUBY18_H

#define MajordomoEncode(str) str

#ifndef RSTRING_PTR
#define RSTRING_PTR(str) RSTRING(str)->ptr
#endif
#ifndef RSTRING_LEN
#define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#include "rubyio.h"
#include "rubysig.h"

/*
 * partial emulation of the 1.9 rb_thread_blocking_region under 1.8,
 * this is enough for dealing with blocking I/O functions in the
 * presence of threads.
 */

#define RUBY_UBF_IO ((rb_unblock_function_t *)-1)
typedef void rb_unblock_function_t(void *);
typedef VALUE rb_blocking_function_t(void *);
static VALUE
rb_thread_blocking_region(
  rb_blocking_function_t *func, void *data1,
  MAJORDOMO_UNUSED rb_unblock_function_t *ubf,
  MAJORDOMO_UNUSED void *data2)
{
  VALUE rv;
  TRAP_BEG;
  rv = func(data1);
  TRAP_END;
  return rv;
}

#endif