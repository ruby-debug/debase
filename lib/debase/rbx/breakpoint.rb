module Debase
  class Breakpoint
    @@global_id = 1

    attr_accessor :source, :pos
    attr_reader :id

    def initialize(file, line, expr=nil)
      @source = file
      @pos = line
      @expr = expr
      @id = @@global_id
      @@global_id = @@global_id + 1
    end
  end
end