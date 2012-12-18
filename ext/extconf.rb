require "mkmf"

# Allow use customization of compile options. For example, the
# following lines could be put in config_options to to turn off
# optimization:
#   $CFLAGS='-fPIC -fno-strict-aliasing -g3 -ggdb -O2 -fPIC'
config_file = File.join(File.dirname(__FILE__), 'config_options.rb')
load config_file if File.exist?(config_file)

dir_config("ruby")
create_makefile("debase_internals")
