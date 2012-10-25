#!/usr/bin/ruby

require "yaml"

# Files
status_file="/var/lib/thermobzizou/status.yaml"

status=YAML::load( File.open(status_file) )

if ARGV[0].nil?
then
  status.keys.each do |key|
    print key + ":" + status[key].to_s + " "
  end
else
  puts status[ARGV[0]]
end
