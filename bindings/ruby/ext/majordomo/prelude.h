#ifndef MAJORDOMO_PRELUDE_H
#define MAJORDOMO_PRELUDE_H
#include <stdint.h>

#ifndef RFLOAT_VALUE
#define RFLOAT_VALUE(v) (RFLOAT(v)->value)
#endif

#ifdef RUBINIUS
#include "rubinius.h"
#else
#ifdef JRUBY
#include "jruby.h"
#else
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
#include "ruby19.h"
#else
#include "ruby18.h"
#endif
#endif
#endif

#endif