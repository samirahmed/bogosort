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
def log(id,delay,cmdstr,verbose,fd)
  time = Time.now.strftime("%X:%L")
  dstr = (delay.to_f*1000).to_i.to_s.blue
  puts "#{id}".pink+"\t| #{dstr} \t| #{time}"+"\t| #{cmdstr}".yellow if (verbose)
  fd.puts "#{id}".pink+"\t| #{dstr} \t| #{time}"+"\t| #{cmdstr}".yellow 
end

def rand_aisle
  if (rand(2)==0)
    "#{rand(180)+10} #{[49,50,149,150].sample}"
  else
    "#{[51,52,151,152].sample} #{rand(180)+10}"
  end
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
    when "TEL" then 
      case arg.strip
        when "?" then "move #{rand_aisle}"
        else "move #{arg.strip}"
      end
    when "MOV" then
      case arg.strip
        when "0" then "up"
        when "1" then "right"
        when "2" then "down"
        when "3" then "left"
        when "up" then "up"
        when "right" then "right"
        when "down" then "down"
        when "left" then "left"
        else nil
      end
    when "DRP" then
      case arg
        when "0" then "drop flag"
        when "1" then "drop shovel"
        when "flag" then "pickup flag"
        when "shovel" then "pickup shovel"
        else  nil
      end
    when "PCK" then
      case arg
        when "0" then "pickup flag"
        when "1" then "pickup shovel"
        when "flag" then "pickup flag"
        when "shovel" then "pickup shovel"
        else nil
      end
    else nil
  end
end

def hash_files( texts, asciis , sfiles)
   
    st,sa = nil,nil
    if (sfiles and !sfiles.empty?)
      st = Digest::MD5.hexdigest(File.read sfiles.first)
      sa = Digest::MD5.hexdigest(File.read sfiles.last)
    end

    thash = texts.map{|fname| Digest::MD5.hexdigest(File.read(fname))}
    ahash = asciis.map{|fname| Digest::MD5.hexdigest(File.read(fname))}
      
    if thash.sort.uniq.size == 1
      puts  "#{SYMBOL_TICK.green} CLIENTS Text-Dump Checksum Match"
      
      if (st)
        if (thash.first == st)
          puts  "#{SYMBOL_TICK.green} CLIENTS+SERVER Textdump Match"
        else
          puts "#{SYMBOL_CROSS.red} CLIENTS+SERVER Text-Dump DO NOT Match "
        end
      end
    
    else
      puts "#{SYMBOL_CROSS.red} CLIENTS Text-Dump Checksum DO NOT Match "
    end
    
    if ahash.sort.uniq.size ==  1
      puts  "#{SYMBOL_TICK.green} CLIENTS ASCII-Dump Checksum Match"
      if (sa)
        if (ahash.first == sa)
          puts  "#{SYMBOL_TICK.green} CLIENTS+SERVER ASCII Match"
        else
          puts "#{SYMBOL_CROSS.red} CLIENTS+SERVER ASCII DO NOT Match "
        end
      end
    else
      puts "#{SYMBOL_CROSS.red} CLIENTS ASCII-Dump Checksum DO NOT Match "
    end
end

def console()
  puts "Welcome to the chef console"
  puts "How many clients? ".yellow
  clients = $stdin.gets.strip.to_i
  puts "How teleport clients to aisles? [y/n]".yellow
  teleport = $stdin.gets.strip.downcase[0] == "y"
  puts "How many moves per client?".yellow
  moves =  $stdin.gets.strip.to_i
  puts "Min delay between requests?".yellow
  mindelay = $stdin.gets.strip.to_i
  puts "Max delay between requests?".yellow
  maxdelay = $stdin.gets.strip.to_i
  puts "Issue goodbye for clients? [y/n]".yellow + 
       "\n(Server will looses Player Position info)"
  goodbyes = $stdin.gets.strip.downcase[0] == "y"
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
  
  if teleport
    cids.shuffle.each{|id| fd.puts "#{id},\t#{delay.call()},\tTEL, ?"}
  end
  
  (cids*moves).shuffle.each{|id| fd.puts "#{id},\t#{delay.call()},\tMOV,\t#{rand(4)}"}
  cids.shuffle.each{|id| fd.puts "#{id},\t#{delay.call()},\tBYE"} if goodbyes
  fd.close
  puts "Recipe Saved to test/#{name}.recipe".green
end

def wait(stage)
   puts "PAUSING".blue + " - #{stage}"
   puts "(hit enter to terminate clients)"
   $stdin.gets
end

def test(arguments)
  # setup paths
  client  = File.join(FileUtils.pwd,'client/client')
  server  = File.join(FileUtils.pwd,'server/server')

  # check if client/server exist
  abort("Error! Executable client/client missing".red) unless File.exists?(client)
  abort("Error! Executable server/server missing".red) unless File.exists?(server)

  ip = 'localhost'
  port, recipe= nil,nil
  pids,fds = [],[]
  kill,slow, debug,client_only,verbose, pause= [false]*6

  arguments.each do |arg|
    if  arg == "--verbose" || arg == "-v"
      verbose = true
    elsif arg == "--slow" || arg == "-s"
      slow = true
    elsif arg == "--pause" || arg == "-p"
      pause = true
    elsif arg == "--client" || arg == "-c"
      client_only = true 
    elsif (arg == "--debug" || arg == "-d")
      debug = true 
    elsif (iparg = arg.scan(/(?:[0-9]{1,3}\.){3}[0-9]{1,3}/).first)
      ip = iparg
      client_only = true 
    elsif (portarg = arg.scan(/\d{4,5}/).first)
      port = portarg
      client_only = true 
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
    
    trap("INT") do 
      kill = true
      raise "SAFE SHUTDOWN TRIGGERED".yellow
    end
    
    puts "Test #{File.basename(recipe).upcase.yellow}"
    fds << File.open(tlog,'w')

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
    instructions = CSV.readlines(recipe)
    params = instructions.shift.map!(&:strip)
    count = params.first.to_i;
    teleport = true if params.include?("teleport")
   
    if teleport && swrite
      swrite.puts "teleport"
      sleep 0.1
    end

    verbose = true if instructions.length < 10

    # create clients
    ios = count.times.map{ IO.pipe }
    
    cpids,clients,io_pids = [],[],[]
    ios.each do |io|
      fd = File.open(clog,'w');
      pid = Process.spawn("#{client}","-no_ui",
        :in=>io.first,:err=>[fd,"a"],:out=>[fd,"a"])
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
        log(id,delay,cmdstr,verbose,fds.first)
        clients[id.to_i].puts cmdstr
        raise "Bad recipe" unless cmdstr
      end
    end

    puts "-"*50 if verbose
    
    wait("before dumping") if pause
  
    puts "DUMP".blue if verbose 
    # wait 50 ms per client 
    sleep 1.0+(count.to_f*0.05)
    sleep 1.0+(count.to_f*0.05) if slow

    # Dump and Hash after a brief pause
    thash,ahash,server_files = [],[]
    io_pids.push([swrite,spid]) unless client_only
    io_pids.each do |stdin,pid| 
      
      is_server = !client_only && spid == pid
    
      # dump logs
      textdump  = File.join log_dir,"#{pid}.text.dump"
      asciidump = File.join log_dir,"#{pid}.ascii.dump"
     
      # if server, give it a seconds the continue
      if is_server
        textdump.gsub!( /(#{pid})/, "server") 
        asciidump.gsub!(/(#{pid})/, "server") 
        server_files = [textdump,asciidump]
        sleep 1 
      end

      
      stdin.puts "textdump #{textdump}"
      stdin.puts "asciidump #{asciidump}"
      
      if is_server
        sleep (slow ? 2 : 0.2)
      else
        thash << textdump
        ahash << asciidump
      end
    end

    wait("after dumping") if pause

    # quit everything 
    io_pids.each{|stdin,pid| stdin.puts "quit" ; Process.waitpid(pid);}
    
    hash_files(thash,ahash,server_files);
    

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
    abort() if kill
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

