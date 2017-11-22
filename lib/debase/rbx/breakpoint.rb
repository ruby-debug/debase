module Debase
  class Breakpoint
    @@global_id = 1

    attr_accessor :source, :pos, :expr, :enabled
    attr_reader :id

    def initialize(file, line, expr=nil)
      @source = file
      @pos = line
      @expr = expr
      @id = @@global_id
      @@global_id = @@global_id + 1
    end

    def delete!

    end

    def self.remove(breakpoints, id)
      bp = breakpoints.delete_if {|b| b.id == id}
      bp.delete! if bp
      bp
    end
  end
end