#!/usr/bin/ruby
require 'csv'
require 'fileutils'
require 'digest/md5'
    
SYMBOL_TICK="\xe2\x9c\x94"
SYMBOL_CROSS="\xe2\x9c\x97"

# monkey patch string for color
class String
  # colorization
  def colorize(color_code)
    "\e[#{color_code}m#{self}\e[0m"
  end
  
  def alt_blue
    colorize(94)
  end
  
  def blue
    colorize(34)
  end

  def red
    colorize(91)
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

## Logging helper
def log(id,cmdstr,verbose,fd)
  time = Time.now.to_s
  puts "#{id}".pink+"\t| #{time}"+"\t| #{cmdstr}".yellow if (verbose)
  fd.write("#{id}\t| #{time}\t| #{cmdstr}\n")
end

## Command parser
def mkcmd(cmd,arg)
  cmd.strip if cmd
  arg.strip if arg
  case cmd.strip
    when "HEL" then "hello"
    when "BYE" then "disconnect"
    when "CDP" then "clientdump"
    when "SDP" then "serverdump"
    when "MOV" then
      case arg.strip
        when "0" then "move up"
        when "1" then "move right"
        when "2" then "move down"
        when "3" then "move left"
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

def console()
  puts "Welcome to the chef console"
  puts "How many clients? ".yellow
  clients = $stdin.gets.strip.to_i
  puts "How many moves per client?".yellow
  moves =  $stdin.gets.strip.to_i
  puts "min delay between requests?".yellow
  mindelay = $stdin.gets.strip.to_i
  puts "max delay between requests?".yellow
  maxdelay = $stdin.gets.strip.to_i
  puts "What do you want to call this recipe?".yellow
  name = $stdin.gets.strip
  
  fname = File.join(FileUtils.pwd,"test/#{name}.recipe")
  
  if File.exists?(fname)
    puts "#{name} already exists! Overwrite it [y/n]?".red
    ow = $stdin.gets.strip.downcase[0] == "y"
    return unless ow
  end
  
  delay = lambda{ return (rand(maxdelay-mindelay)+mindelay)/1000.0 }
  cids = (0...clients).to_a
  fd = File.open(fname,'w');

  fd.puts "#{clients}";
  cids.shuffle.each{|id| fd.puts "#{id},\t#{delay.call()},\tHEL"}
  (cids*moves).shuffle.each{|id| fd.puts "#{id},\t#{delay.call()},\tMOV,\t#{rand(4)}"}
  cids.shuffle.each{|id| fd.puts "#{id},\t#{delay.call()},\tBYE"}
  fd.close
  puts "Recipe Saved to test/#{name}.recipe".green
end

def test(arguments)
  # setup paths
  client  = File.join(FileUtils.pwd,'client/client')
  server  = File.join(FileUtils.pwd,'server/server')

  # check if client/server exist
  abort("Error! Executable client/client missing".red) unless File.exists?(client)
  abort("Error! Executable server/server missing".red) unless File.exists?(server)

  ip = 'localhost'
  port, recipe = nil,nil
  pids,fds = [],[]
  debug,client_only,verbose, checksum = false,false,false,false

  arguments.each do |arg|
    if  arg == "--verbose" || arg == "-v"
      verbose = true
    elsif arg == "--checksum" || arg == "-x"
      checksum = true
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
  log_dir = File.join(FileUtils.pwd,"log/#{File.basename(recipe,".*")}")
  FileUtils.rm_rf File.join(log_dir), :verbose=>debug
  FileUtils.mkdir_p log_dir,:verbose=>debug
  slog = File.join log_dir,'server.log'
  clog = File.join log_dir,'client.log'
  tlog = File.join log_dir,'test.log'

  begin
    
    puts "Test #{File.basename(recipe).upcase.yellow}"
    fds << File.open(tlog,'w')
    
    trap("INT"){ puts "Hold your horses".yellow; raise "SIGINT";}

    # start a server
    if not client_only
      fd  =  File.open(slog,'w')
      sread, swrite = IO.pipe
      spid = Process.spawn(server, :err=>fd , :out=>fd, :in=>sread )
      pids << spid
      fds  << fd
       
      puts "Launching local server ... " if debug
      sleep 0.5
      
      puts "sh -c #{server}" if (debug) 
      port = File.readlines("#{slog}")
                 .join("\n").scan(/RPC Port: \d{4,5}/).first
                 .scan(/\d+/).first

      raise "Error! Can't parse port: #{port}".red unless port
    end
   
    raise "Error! Can't find port".red unless port
    puts "#{ip.alt_blue}:#{port.blue}"
    
    # read the recipe file
    instructions = CSV.readlines(recipe);
    count = instructions.shift.first.to_i;
    
    verbose = true if instructions.length < 10

    # create clients
    ios = count.times.map{ IO.pipe }
    
    cpids,clients,io_pids = [],[],[]
    ios.each do |io|
      fd = File.open(clog,'w');
      pid = Process.spawn("#{client}",:in=>io.first,:err=>[fd,"a"],:out=>[fd,"a"])
      cpids << pid
      pids << pid
      fds << fd
      clients << io.last  
      io_pids << [io.last,pid]
    end
    
    puts "Clients #{cpids.inspect}"

    # connect all clients 
    clients.each{|stdin| stdin.puts "connect #{ip}:#{port}"}
    
    puts "-"*50 if verbose 
    
    # exec instructions
    instructions.each do |instruction|
      puts instruction.join(" ") if debug
      id, delay, cmd, args = instruction
      
      if ( id && delay && cmd)
        sleep delay.to_f
        cmdstr = mkcmd(cmd,args)
        log(id,cmdstr,verbose,fds.first)
        clients[id.to_i].puts cmdstr
        raise "Bad recipe" unless cmdstr
      end
    end

    puts "-"*50 if verbose
  
    puts "Dumping and hashing" if verbose 
    # terminate clients
    thash,ahash = [],[]
    io_pids.push([swrite,spid]) unless client_only
    io_pids.each do |stdin,pid| 
      
      is_server = !client_only && spid == pid
    
      # dump logs
      textdump  = File.join log_dir,"#{pid}.text.dump"
      asciidump = File.join log_dir,"#{pid}.ascii.dump"
     
      # if server, give it a seconds the continue
      textdump.gsub! /(#{pid})/, "server"  if is_server
      asciidump.gsub! /(#{pid})/, "server" if is_server
      sleep 1 if is_server
      
      stdin.puts "textdump #{textdump}"
      stdin.puts "asciidump #{asciidump}"

      thash << textdump
      ahash << asciidump
    end

    io_pids.each{|stdin,pid| stdin.puts "quit" ; Process.waitpid(pid);}
    
    # hash
    thash.map!{|fname| Digest::MD5.hexdigest(File.read(fname))}
    ahash.map!{|fname| Digest::MD5.hexdigest(File.read(fname))}
      
    if thash.sort.uniq.size == 1
      puts  "#{SYMBOL_TICK.green} Text-Dump Checksum Match"
    else
      puts "#{SYMBOL_CROSS.red} Text-Dump Checksum DO NOT Match "
    end
    
    if ahash.sort.uniq.size ==  1
      puts  "#{SYMBOL_TICK.green} ASCII-Dump Checksum Match"
    else
      puts "#{SYMBOL_CROSS.red} ASCII-Dump Checksum DO NOT Match "
    end

  rescue 
    puts "Error! Suspending Execution and killing children"
    puts  $!, $@
  ensure
    puts "Cleaning Process & Closing FDs"

    pids.each do |pid| 
      begin 
        Process.kill(15,pid); 
      rescue Errno::ESRCH
        nil
      end
    end
    fds.each{|file| file.close }
    puts ""
  end
end

def main
  if ARGV.include?("console")
    console()
  elsif ARGV.include?("ALL") || ARGV.include?("--all") || ARGV.include?("-a")
    arguments = Array.new(ARGV)
    arguments -= ["--all","ALL","-a"]
    Dir.glob("./test/*.recipe").map{|r| test(arguments+[r])}
  else
    test(ARGV)
  end
end

main

