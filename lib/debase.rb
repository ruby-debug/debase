if defined?(RUBY_ENGINE) && 'rbx' == RUBY_ENGINE
  require 'debase/rbx'
else
  require "debase_internals"
end
require "debase/version"
require "debase/context"

module Debase
  class << self
    attr_accessor :handler

    alias start_ setup_tracepoints
    alias stop remove_tracepoints

    # possibly deprecated options
    attr_accessor :keep_frame_binding, :tracing

    def start(options={}, &block)
      Debugger.const_set('ARGV', ARGV.clone) unless defined? Debugger::ARGV
      Debugger.const_set('PROG_SCRIPT', $0) unless defined? Debugger::PROG_SCRIPT
      Debugger.const_set('INITIAL_DIR', Dir.pwd) unless  defined? Debugger::INITIAL_DIR

      monkey_patch_prepend

      Debugger.started? ? block && block.call(self) : Debugger.start_(&block)
    end

    def monkey_patch_prepend
      class << RubyVM::InstructionSequence
        def self.prepend(mod, *smth)
          super
          if mod.to_s.include?('Bootsnap') && RUBY_VERSION >= '2.5' && RUBY_VERSION < '2.6'
            prepend InstructionSequenceMixin
          end
        end
      end
    end

    # @param [String] file
    # @param [Fixnum] line
    # @param [String] expr
    def add_breakpoint(file, line, expr=nil)
      breakpoint = Breakpoint.new(file, line, expr)
      breakpoints << breakpoint
      enable_trace_points
      breakpoint
    end

    def remove_breakpoint(id)
      Breakpoint.remove breakpoints, id
    end

    def source_reload; {} end

    def post_mortem?
      false
    end

    def debug
      false
    end

    def add_catchpoint(exception)
      catchpoints[exception] = 0
      enable_trace_points
    end

    def remove_catchpoint(exception)
      catchpoints.delete(exception)
    end

    def clear_catchpoints
      catchpoints.clear
    end

    #call-seq:
    #   Debase.skip { block } -> obj or nil
    #
    #The code inside of the block is escaped from the debugger.
    def skip
      #it looks like no-op is ok for this method for now
      #no-op
    end

    #call-seq:
    #   Debugger.last_interrupted -> context
    #
    #Returns last debugged context.
    def last_context
      # not sure why we need this so let's return nil for now ;)
      nil
    end

    def file_filter
      @file_filter ||= FileFilter.new
    end

    module InstructionSequenceMixin
      def load_iseq(path)
        iseq = super(path)

        do_set_flags(iseq)

        iseq
      end

      def do_set_flags(iseq)
        Debugger.set_trace_flag_to_iseq(iseq)
        iseq.each_child { |child_iseq| do_set_flags(child_iseq) } if iseq.respond_to? :each_child
      end
    end
  end

  class FileFilter
    def initialize
      @enabled = false
    end

    def include(file_path)
      included << file_path unless excluded.delete(file_path)
    end

    def exclude(file_path)
      excluded << file_path unless included.delete(file_path)
    end

    def enable
      @enabled = true
      Debase.enable_file_filtering(@enabled);
    end

    def disable
      @enabled = false
      Debase.enable_file_filtering(@enabled);
    end

    def accept?(file_path)
      return true unless @enabled
      return false if file_path.nil?
      included.any? { |path| file_path.start_with?(path) } && excluded.all? { |path| !file_path.start_with?(path)}
    end

    private
    def included
      @included ||= []
    end

    def excluded
      @excluded ||= []
    end
  end

  class DebugThread < Thread
    def self.inherited
      raise RuntimeError.new("Can't inherit Debugger::DebugThread class")
    end  
  end
end

Debugger = Debase
