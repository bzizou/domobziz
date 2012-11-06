#!/usr/bin/ruby
# Little daemon to manage temperature setting and status
# of the thermobzizou arduino software
# This daemon is aimed to act as a gateway between the
# arduino and a web interface.
# If the daemon stops, the arduino stills continue to
# regulate the temperature using the default setpoint (tcur)

require "rubygems"
require "serialport"
require "yaml"

# Serial port setup
port_str = "/dev/ttyACM0"
baud_rate = 9600
data_bits = 8
stop_bits = 1
parity = SerialPort::NONE

# Files
t_file="/var/lib/thermobzizou/t.dat" # Contains the temperature to set
r_file="/var/lib/thermobzizou/r.dat" # Contains the index of the ref temperature
status_file="/var/lib/thermobzizou/status.yaml"

# Ambiant temperature setpoint in centigrade degrees
tcur='19' # Set default value here
rcur='1'

# Init serial port
sp = SerialPort.new(port_str, baud_rate, data_bits, stop_bits, parity)
#sp.dtr=0 # Should prevent reset at connexion, but inefficient :-(
sp.read_timeout=-1;

# Wait a bit for arduino to boot and execute setup
sleep(5)
trash=sp.read

# Main loop of the daemon
values={}
while true do

  # Set ambiant temperature setpoint
  t=File.read(t_file).chomp.to_i
  if t!=tcur 
    puts "Sending new setpoint #{t.to_s}"
    sp.write("T"+t.to_s)
    tcur=t
    sleep(8)
    puts sp.read
  end   

  # Set reference temperature
  r=File.read(r_file).chomp.to_i
    if r!=rcur
    puts "Sending new ref temp #{r.to_s}"
    sp.write("R0"+r.to_s)
    rcur=r
    sleep(8)
    puts sp.read
  end

  # Update status file
  sp.write("P")
  sleep(8)
  sp.read.split(/\n/).each do |entry|
    entries=entry.split(/:\s*/)
    values[entries[0]]=entries[1]
  end
  if values["SA"].is_a?(Integer)
    tcur=values["SA"].to_i
  end
  values['DATE']=Time.now().to_i
  values['DATE_HUMAN']=Time.now().to_s
  File.open(status_file,"w") {|f| f.write(values.to_yaml)}

  if values["AL"].to_s == "1"
    puts "Alarm!: "+Time.now()
  end
   
  # Main loop timer
  $stdout.flush
  sleep(15);
end

sp.close
