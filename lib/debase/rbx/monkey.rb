class Object
  # todo: remove me, I'm here only for debugging
  def method_missing(name, *args)
    puts "#{self.class}.#{name}"
    super(name, args)
  end
end

module Rubinius
  class Location
    attr_reader :receiver
  end
end
