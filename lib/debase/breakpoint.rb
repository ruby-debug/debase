module Debase
  class Breakpoint
    attr_reader :source, :pos, :id
    attr_accessor :expr

    def initialize(id, source, pos, expr)
      @id = id
      @source = source
      @pos = pos
      @expr = expr
    end

    # @param [Array<Breakpoint>] breakpoints
    def self.remove(breakpoints, id)
      breakpoints.each do |breakpoint|
        if breakpoint.id == id
          breakpoints.delete breakpoint
          return
        end
      end
    end

    # @param [Array<Breakpoint>] breakpoints
    # @return [Breakpoint]
    def self.find(breakpoints, source, pos, binding)
      breakpoints.each do |breakpoint|
        if breakpoint.source == source && breakpoint.pos == pos
          return breakpoint
        end
      end
      nil
    end
  end
end