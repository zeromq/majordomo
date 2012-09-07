# encoding: utf-8

require 'rubygems' unless defined?(Gem)
require 'rake' unless defined?(Rake)

# Prefer compiled Rubinius bytecode in .rbx/
ENV["RBXOPT"] = "-Xrbc.db"

require 'rake/extensiontask'
require 'rake/testtask'

begin
require 'rdoc/task'
rescue LoadError # fallback to older 1.8.7 rubies
require 'rake/rdoctask'
end

gemspec = eval(IO.read('mdp.gemspec'))

Gem::PackageTask.new(gemspec) do |pkg|
end

Rake::ExtensionTask.new('majordomo', gemspec) do |ext|
  ext.name = 'majordomo_ext'
  ext.ext_dir = 'ext/majordomo'

  CLEAN.include 'lib/**/majordomo_ext.*'
end

desc 'Run Majordomo tests'
Rake::TestTask.new(:test) do |t|
  t.test_files = Dir.glob("test/**/test_*.rb")
  t.verbose = true
  t.warning = true
end

Rake::RDocTask.new do |rd|
  files = FileList["README.rdoc", "ext/majordomo/*.c"]
  rd.title = "A Ruby binding for libmd (Majordomo implementation in C)"
  rd.main = "README.rdoc"
  rd.rdoc_dir = "doc"
  rd.options << "--promiscuous"
  rd.rdoc_files.include(files)
end

task :test => :compile
task :default => :test