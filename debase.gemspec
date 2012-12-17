# -*- encoding: utf-8 -*-
$:.push File.expand_path("../lib", __FILE__)
require "debase/version"

Gem::Specification.new do |s|
  s.name        = "debase"
  s.version     = Debase::VERSION
  s.authors     = ["Dennis Ushakov"]
  s.email       = ["dennis.ushakov@gmail.com"]
  s.homepage    = ""
  s.summary     = %q{TODO: Write a gem summary}
  s.description = <<-EOF
    ruby-debug is a fast implementation of the standard Ruby debugger debug.rb.
    It is implemented by utilizing a new Ruby C API hook. The core component
    provides support that front-ends can build on. It provides breakpoint
    handling, bindings for stack frames among other things.
  EOF

  s.rubyforge_project = "debase"

  s.files         = `git ls-files`.split("\n")
  s.test_files    = `git ls-files -- {test,spec,features}/*`.split("\n")
  s.executables   = `git ls-files -- bin/*`.split("\n").map{ |f| File.basename(f) }
  s.require_paths = ["lib"]

  s.extensions = ["ext/extconf.rb"]

  # specify any dependencies here; for example:
  s.add_development_dependency "test-unit"
  s.add_development_dependency "rake"
  # s.add_runtime_dependency "rest-client"
end
