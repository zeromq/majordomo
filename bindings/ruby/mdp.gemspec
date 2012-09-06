# encoding: utf-8

require File.expand_path('../lib/majordomo/version', __FILE__)

Gem::Specification.new do |s|
  s.name = "mdp"
  s.version = Majordomo::VERSION
  s.summary = "Majordomo for Ruby"
  s.description = "Majordomo - A Ruby binding for libmdp (Majordomo implementation in C)"
  s.authors = ["Lourens Naudé"]
  s.email = ["lourens@methodmissing.com"]
  s.homepage = "http://github.com/methodmissing/majordomo"
  s.date = Time.now.utc.strftime('%Y-%m-%d')
  s.platform = Gem::Platform::RUBY
  s.extensions = "ext/majordomo/extconf.rb"
  s.has_rdoc = true
  s.files = `git ls-files`.split("\n")
  s.test_files = `git ls-files test`.split("\n")
  s.rdoc_options = ["--charset=UTF-8"]
  s.require_paths = ["lib"]
  s.add_development_dependency('rake-compiler', '~> 0.8.0')
end