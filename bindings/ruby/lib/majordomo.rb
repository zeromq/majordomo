# encoding: utf-8

# Prefer compiled Rubinius bytecode in .rbx/
ENV["RBXOPT"] = "-Xrbc.db"

require "majordomo_ext"

require 'majordomo/version' unless defined? Majordomo::VERSION