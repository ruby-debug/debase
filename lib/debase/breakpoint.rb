module Debase
  class Breakpoint
    attr_reader :file, :line

    def initialize(id, file, line)
      @bid = id
      @file = file
      @line = line
    end

    def bid
      @bid
    end

    # @param [Array<Breakpoint>] breakpoints
    def self.remove(breakpoints, id)
      breakpoints.each do |breakpoint|
        if breakpoint.bid == id
          breakpoints.delete breakpoint
          return
        end
      end
    end

    # @param [Array<Breakpoint>] breakpoints
    # @return [Breakpoint]
    def self.find(breakpoints, file, line)
      breakpoints.each do |breakpoint|
        if breakpoint.file == file && breakpoint.line == line
          return breakpoint
        end
      end
      nil
    end
  end
end