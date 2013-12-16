module Debase
  class Frame
    def initialize(location)
      @location = location
    end

    def binding
      @binding ||= Binding.setup(@location.variables, @location.method, @location.constant_scope)
    end

    def line
      @location.line
    end

    def file
      @location.file
    end

    def self
      @location.receiver
    end
  end
end
