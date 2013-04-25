#!/usr/bin/ruby
require 'csv'
require 'fileutils'
require 'digest/md5'

# monkey patch string for color
class String
  # colorization
  def colorize(color_code)
    "\e[#{color_code}m#{self}\e[0m"
  end

  def red
    colorize(31)
  end

  def green
    colorize(32)
  end

  def yellow
    colorize(33)
  end

  def pink
    colorize(35)
  end
end

def mkcmd(cmd,arg)
  cmd.strip if cmd
  arg.strip if arg
  case cmd.strip
    when "HEL" then "hello"
    when "BYE" then "disconnect"
    when "MOV" then
      case arg.strip
        when "0" then "move up"
        when "1" then "move right"
        when "2" then "move left"
        when "3" then "move down"
        else nil
      end
    when "DRP" then
      case arg
        when "0" then "drop flag"
        when "1" then "drop shovel"
        else  nil
      end
    when "PCK" then
      case arg
        when "0" then "pickup flag"
        when "1" then "pickup shovel"
        else nil
      end
    else nil
  end
end

# setup paths
client  = File.join(FileUtils.pwd,'client/client')
server  = File.join(FileUtils.pwd,'server/server')

# check if client/server exist
abort("Error! No client found".red) unless File.exists?(client)
abort("Error! No server found".red) unless File.exists?(server)

ip = 'localhost'
port, recipe = nil,nil
pids,fds = [],[]
debug,client_only, checksum = false,false,false

ARGV.each do |arg|
  if arg == "--checksum" || arg == "-x"
    dump = true
  elsif arg == "--client" || arg == "-c"
    client_only = true 
  elsif (arg == "--debug" || arg == "-d")
    debug = true 
  elsif (iparg = arg.scan(/(?:[0-9]{1,3}\.){3}[0-9]{1,3}/).first)
    ip = iparg
  elsif (portarg = arg.scan(/\d{4,5}/).first)
    port = portarg
  else
    recipe = arg
  end 
end

abort("Error! No test recipe specified".red) unless recipe
abort("Error! Can't find recipe ".red) unless File.exists?(recipe)

# prepare log files
log_dir = File.join(FileUtils.pwd,'log/')
slog    = File.join(log_dir,File.basename(recipe,".*")+'-server.log');
clog    = File.join(log_dir,File.basename(recipe,".*")+'-client.log');
tlog    = File.join(log_dir,File.basename(recipe,".*")+'-test.log');

begin

  trap("INT"){ puts "Hold your horses".yellow; raise "SIGINT";}

  # start a server
  if not client_only
    fd  =  File.open(slog,'w')
    pids << Process.spawn(server, :err=>fd , :out=>fd )
    fds  << fd
     
    puts "Launching Local Server pid=#{pids.first}"
    sleep 0.5
    
    puts cmd if (debug) 
    port_str = File.readlines("#{slog}").join("\n").scan(/RPC Port: \d{4,5}/).first;
    port = port_str.scan(/\d+/).first if port_str
    raise "Error! Can't parse server log: #{port_str}".red unless port && port_str
  end
 
  # read the port
  raise "Error! Can't find port".red unless port
  puts "Server on #{ip}:#{port}"
  
  # read the recipe file
  instructions = CSV.readlines(recipe);
  count = instructions.shift.first.to_i;
  # create clients
  ios = count.times.map{ IO.pipe }
  ios.each do |io|
    fd = File.open(clog,'w');
    pids << Process.spawn("#{client}",:in=>io.first,:err=>fd,:out=>fd)
    fds << fd
  end
  puts "Clients #{pids[1..-1].inspect}"
  clients = ios.map{|io| io.last}
  
  # connect all clients 
  clients.each{|stdin| stdin.puts "connect #{ip}:#{port}"}
  
  # exec instructions
  instructions.each do |instruction|
    puts instruction.join(" ") if debug
    id, delay, cmd, args = instruction
    
    if ( id && delay && cmd)
      sleep delay.to_f
      cmdstr = mkcmd(cmd,args)
      puts "#{id}".pink+"| #{Time.now.to_s} | "+"#{cmdstr}".yellow
      clients[id.to_i].puts cmdstr
      raise "Bad recipe" unless cmdstr
    end
  end

  # issue dumps to errbody

  # connect all clients 
  clients.each{|stdin| stdin.puts "quit"}
rescue 
  puts "Error! Suspending Execution and killing children"
  puts  $!, $@
ensure
  puts "Cleaning Process & Closing FDs"
  pids.each{|pid| Process.kill(15,pid) }
  fds.each{|file| file.close }
end
