require 'bundler/gem_tasks'
require 'rake/testtask'

BASE_TEST_FILE_LIST = Dir['test/**/test_*.rb']

# see https://docs.travis-ci.com/user/reference/osx/
# see https://docs.travis-ci.com/user/reference/linux/
# see https://rubies.travis-ci.org/
desc "Generate travis.yml"
task :gen_travis do
  matrix = []

  def matrix.macos(image, *versions)
    versions.each { |version| self << ['osx', 'osx_image', image, version] }
  end

  def matrix.linux(distro, *versions)
    versions.each { |version| self << ['linux', 'dist', distro, version] }
  end

  since_24 = %w[2.4 2.5 2.6 2.7 3.0 3.1 3.2 3.3 3.4-preview2].reverse
  since_23 = [*since_24, '2.3']
  since_22 = [*since_23, '2.2']
  since_21 = [*since_22, '2.1']

  # Ubuntu 16.04
  matrix.linux 'xenial', *since_21
  # Ubuntu 18.04
  matrix.linux 'bionic', *since_23
  # Ubuntu 20.04
  matrix.linux 'focal', *since_24
  # macOS 10.14.6
  matrix.macos 'xcode11.3', *since_24
  # macOS 10.15.7
  matrix.macos 'xcode12.2', *since_24, 'ruby-head'

  puts <<'EOM'
language: ruby
matrix:
  fast_finish: true
  include:
EOM

  matrix.each do |data|
    puts <<EOM
    - os: #{data[0]}
      #{data[1]}: #{data[2]}
      rvm: #{data[3]}
EOM
  end

end

desc "Remove built files"
task :clean do
  cd "ext" do
    if File.exist?("Makefile")
      sh "make clean"
      rm "Makefile"
    end
    derived_files = Dir.glob(".o") + Dir.glob("*.so") + Dir.glob("*.bundle")
    rm derived_files unless derived_files.empty?
  end
  cd "ext/attach" do
    if File.exist?("Makefile")
      sh "make clean"
      rm "Makefile"
    end
    derived_files = Dir.glob(".o") + Dir.glob("*.so") + Dir.glob("*.bundle")
    rm derived_files unless derived_files.empty?
  end
  if File.exist?('pkg')
    cd 'pkg' do
      derived_files = Dir.glob('*.gem')
      rm derived_files unless derived_files.empty?
    end
  end
end

desc "Create the core debase shared library extension"
task :lib => :clean do
  Dir.chdir("ext") do
    system("#{Gem.ruby} extconf.rb && make")
    exit $?.exitstatus if $?.exitstatus != 0
  end
  Dir.chdir("ext/attach") do
    system("#{Gem.ruby} extconf.rb && make")
    exit $?.exitstatus if $?.exitstatus != 0
  end
end

desc "Test debase."
Rake::TestTask.new(:test) do |t|
  t.libs += ['./ext', './lib']
  t.test_files = FileList[BASE_TEST_FILE_LIST]
  t.verbose = true
end
task :test => :lib

task :default => :test
