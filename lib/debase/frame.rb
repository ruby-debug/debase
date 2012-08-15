module Debase
  class Frame
    attr_accessor :file, :line, :bind, :klass

    def initialize(file, line, binding, klass)
      @file = file
      @line = line
      @bind = binding
      @klass = klass
    end

    def to_s
      "#{file}:#{line}(#{klass})"
    end
  end
end