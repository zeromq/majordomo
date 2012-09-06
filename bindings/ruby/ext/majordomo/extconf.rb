# encoding: utf-8

require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

def fail(msg)
  STDERR.puts msg
  exit(1)
end

dir_config('majordomo')

fail("libmdp header file 'mdp.h' not found") unless have_header('mdp.h')

fail("zmq not found") unless have_library('zmq')
fail("czmq not found") unless have_library('czmq')
fail("libdmp not found") unless have_library('mdp')

have_func('rb_thread_blocking_region')

$CFLAGS << ' -Wall -funroll-loops'
$CFLAGS << ' -Wextra -O0 -ggdb3' if ENV['DEBUG']

create_makefile('majordomo_ext')