# -*- encoding: utf-8 -*-
$:.push File.expand_path("../lib", __FILE__)
require "debase/version"

Gem::Specification.new do |s|
  s.name        = "debase"
  s.version     = Debase::VERSION
  s.license     = "MIT"
  s.authors     = ["Dennis Ushakov"]
  s.email       = ["dennis.ushakov@gmail.com"]
  s.homepage    = "https://github.com/denofevil/debase"
  s.summary     = %q{debase is a fast implementation of the standard Ruby debugger debug.rb for Ruby 2.0}
  s.description = <<-EOF
    debase is a fast implementation of the standard Ruby debugger debug.rb for Ruby 2.0.
    It is implemented by utilizing a new Ruby TracePoint class. The core component
    provides support that front-ends can build on. It provides breakpoint
    handling, bindings for stack frames among other things.
  EOF

  s.rubyforge_project = "debase"

  s.files         = `git ls-files`.split("\n")
  s.test_files    = `git ls-files -- {test,spec,features}/*`.split("\n")
  s.executables   = `git ls-files -- bin/*`.split("\n").map{ |f| File.basename(f) }
  s.require_paths = ["lib"]

  s.extensions = ["ext/extconf.rb", "ext/attach/extconf.rb"]

  s.required_ruby_version = ">= 2.0"

  s.add_dependency "debase-ruby_core_source", ">= 0.10.2"
  s.add_development_dependency "test-unit"
  s.add_development_dependency "rake"
end
