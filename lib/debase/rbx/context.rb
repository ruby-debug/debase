require 'debase/rbx/frame'

module Debase
	class Context
    @@max_thread_num = 1
    attr_reader :thread, :thnum

    def initialize(thread)
      @thread = thread
      @thnum = @@max_thread_num
      @@max_thread_num = @@max_thread_num + 1
    end

    def frame_file(frame=0)
      @frames[frame].file
    end

    def frame_line(frame=0)
      @frames[frame].line
    end

    def frame_binding(frame=0)
      @frames[frame].binding
    end

    def frame_self(frame=0)
      @frames[frame].self
    end

    def stack_size
      @frames.size
    end

    def fill_frame_info(locations)
      @frames = []
      locations.each do |loc|
        @frames << Frame.new(loc)
      end
      @frames
    end

    def clear_frame_info
      @frames = nil
    end

    def stop_reason
      :breakpoint
    end

    def ignored?
      thread.is_a? DebugThread
    end

    def dead?
      !@thread.alive?
    end
	end
end
