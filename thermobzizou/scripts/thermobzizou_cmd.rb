#!/usr/bin/ruby
# Simple command line to get the data from the status file
# It's designed to be a feed for a graph tool like Cacti

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
