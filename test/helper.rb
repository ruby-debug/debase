require "test-unit"
require "debase"

module Debase
  class << self
    alias start_ setup_tracepoints
    alias stop remove_tracepoints
  end
end