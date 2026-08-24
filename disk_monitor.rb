# disk_monitor.rb
#!/usr/bin/env ruby
require 'json'
require 'optparse'
require 'time'
require 'open3'

CONFIG_FILE = 'disk_monitor_config.json'

class DiskMonitor
  attr_accessor :threshold, :interval, :path

  def initialize
    load_config
    @threshold ||= 80
    @interval ||= 60
    @path ||= RUBY_PLATFORM =~ /mswin|mingw/ ? 'C:\\' : '/'
  end

  def load_config
    if File.exist?(CONFIG_FILE)
      cfg = JSON.parse(File.read(CONFIG_FILE))
      @threshold = cfg['threshold']
      @interval = cfg['interval']
      @path = cfg['path']
    end
  end

  def save_config
    File.write(CONFIG_FILE, JSON.pretty_generate({
      'threshold' => @threshold,
      'interval' => @interval,
      'path' => @path
    }))
  end

  def get_disk_usage
    total = used = free = 0
    if RUBY_PLATFORM =~ /mswin|mingw/
      # Use wmic
      cmd = "wmic logicaldisk where DeviceID=\"#{@path}\" get Size,FreeSpace"
      stdout, stderr, status = Open3.capture3(cmd)
      return nil unless status.success?
      lines = stdout.lines.map(&:strip).reject(&:empty?)
      return nil if lines.size < 2
      parts = lines[1].split(/\s+/)
      total = parts[1].to_i
      free = parts[0].to_i
      used = total - free
    else
      cmd = "df -k \"#{@path}\""
      stdout, stderr, status = Open3.capture3(cmd)
      return nil unless status.success?
      lines = stdout.lines
      return nil if lines.size < 2
      parts = lines[1].split
      total = parts[1].to_i * 1024
      used = parts[2].to_i * 1024
      free = parts[3].to_i * 1024
    end
    return nil if total == 0
    percent = (used.to_f / total) * 100
    { total: total, used: used, free: free, percent: percent }
  end

  def monitor(once: false)
    puts "💾 Disk Monitor"
    puts "Monitoring: #{@path}"
    puts "Threshold: #{@threshold}% | Interval: #{@interval}s\n"

    alerted = false
    loop do
      usage = get_disk_usage
      unless usage
        puts "Error getting disk usage."
        break
      end
      now = Time.now.strftime("%Y-%m-%d %H:%M:%S")
      percent = usage[:percent]
      total_gb = usage[:total] / (1024.0 ** 3)
      used_gb = usage[:used] / (1024.0 ** 3)
      status = percent > @threshold ? "🔔 ALERT" : "✅ OK"
      color = percent > @threshold ? "\e[91m" : "\e[92m"
      puts "[#{now}] #{color}#{status}\e[0m – #{'%.1f' % percent}% used (#{'%.1f' % used_gb} GB / #{'%.1f' % total_gb} GB)"

      if percent > @threshold && !alerted
        alerted = true
      elsif percent <= @threshold
        alerted = false
      end

      break if once
      sleep(@interval)
    end
  end
end

options = {}
OptionParser.new do |opts|
  opts.banner = "Usage: disk_monitor.rb [options]"
  opts.on("--threshold N", Integer, "Alert threshold percentage") { |v| options[:threshold] = v }
  opts.on("--interval N", Integer, "Check interval in seconds") { |v| options[:interval] = v }
  opts.on("--path PATH", "Disk path to monitor") { |v| options[:path] = v }
  opts.on("--once", "Run once and exit") { options[:once] = true }
  opts.on("--save", "Save current settings to config") { options[:save] = true }
end.parse!

monitor = DiskMonitor.new
monitor.threshold = options[:threshold] if options[:threshold]
monitor.interval = options[:interval] if options[:interval]
monitor.path = options[:path] if options[:path]

if options[:save]
  monitor.save_config
  puts "✅ Configuration saved."
  exit 0
end

monitor.monitor(once: options[:once])
