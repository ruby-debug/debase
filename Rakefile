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

  # versions using old openssl
  legacy_versions = %w[2.0 2.1.10 2.2.10 2.3.8].reverse

  since_24 = %w[2.4.10 2.5.8 2.6.6 2.7.2 3.0].reverse
  since_23 = [*since_24, '2.3.8']
  since_22 = [*since_23, '2.2.10']
  since_21 = [*since_22, '2.1.10']

  # Ubuntu 16.04
  matrix.linux 'xenial', *since_21, 'ruby-head'
  # Ubuntu 18.04
  matrix.linux 'bionic', *since_23, 'ruby-head'
  # Ubuntu 20.04
  matrix.linux 'focal', *since_24, 'ruby-head'
  # macOS 10.14.6
  matrix.macos 'xcode11.3', *since_24, 'ruby-head'
  # macOS 10.15.7
  matrix.macos 'xcode12.2', *since_24, 'ruby-head'
  # macOS 10.13 see https://travis-ci.community/t/ruby-2-0-2-3-fails-to-install-at-macos-10-13/11024
  # matrix.macos 'xcode9.4', *legacy_versions

  puts <<EOM
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

  # see https://travis-ci.community/t/ruby-3-0-on-macos-fails-libraries-missing/11011
  puts <<EOM
  allow_failures:
    - os: osx
      rvm: 3.0
    - rvm: ruby-head
EOM

end

desc "Remove built files"
task :clean do
  cd "ext" do
    if File.exists?("Makefile")
      sh "make clean"
      rm "Makefile"
    end
    derived_files = Dir.glob(".o") + Dir.glob("*.so") + Dir.glob("*.bundle")
    rm derived_files unless derived_files.empty?
  end
  cd "ext/attach" do
    if File.exists?("Makefile")
      sh "make clean"
      rm "Makefile"
    end
    derived_files = Dir.glob(".o") + Dir.glob("*.so") + Dir.glob("*.bundle")
    rm derived_files unless derived_files.empty?
  end
  if File.exists?('pkg')
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
