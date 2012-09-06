# encoding: utf-8

require 'test/unit'
require 'majordomo'

class MajordomoTestCase < Test::Unit::TestCase
  undef_method :default_test if method_defined? :default_test

  BROKER = "tcp://localhost:5555"

  if ENV['STRESS_GC']
    def setup
      GC.stress = true
    end

    def teardown
      GC.stress = false
    end
  end
end