
task :default => :mrbtest
#
# build a single big file for
# easier debugging with Mruby
# 
task :mrbpack do
  target_files = []
  
  %w(
    tool.rb
    types_spec.rb
  ).each do |path|
    target_files << File.expand_path("../specs/#{path}", __FILE__)
  end
  
  FileUtils.mkdir_p('tmp')
  cd 'tmp' do
    File.open('blob', 'w') do |f|
      target_files.each do |path|
        f.puts "# #{path} ==============="
        f.write( File.read(path) )
        f.puts
        f.puts
      end
    end
  end
end


task :mrbtest => [:mrbpack] do
  sh 'mruby tmp/blob'
end

