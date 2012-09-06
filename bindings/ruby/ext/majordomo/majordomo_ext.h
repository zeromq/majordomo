#ifndef MAJORDOMO_EXT_H
#define MAJORDOMO_EXT_H

#include <mdp.h>
#include "ruby.h"

/* Compiler specific */

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define MAJORDOMO_UNUSED __attribute__ ((unused))
#define MAJORDOMO_NOINLINE __attribute__ ((noinline))
#else
#define MAJORDOMO_UNUSED
#define MAJORDOMO_NOINLINE
#endif

#include "prelude.h"

extern VALUE rb_mMajordomo;
extern VALUE rb_cMajordomoClient;
extern VALUE rb_cMajordomoWorker;

#include "client.h"
#include "worker.h"

#endif