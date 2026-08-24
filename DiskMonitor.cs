// DiskMonitor.cs
using System;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading;
using System.Collections.Generic;

class Config
{
    [JsonPropertyName("threshold")]
    public int Threshold { get; set; } = 80;
    [JsonPropertyName("interval")]
    public int Interval { get; set; } = 60;
    [JsonPropertyName("path")]
    public string Path { get; set; } = "/";
}

class DiskUsage
{
    public long Total { get; set; }
    public long Used { get; set; }
    public long Free { get; set; }
    public double Percent { get; set; }
}

class DiskMonitor
{
    private static readonly string ConfigFile = "disk_monitor_config.json";
    private static readonly JsonSerializerOptions Options = new JsonSerializerOptions { WriteIndented = true };
    private Config config;

    public DiskMonitor()
    {
        config = LoadConfig();
        if (Environment.OSVersion.Platform == PlatformID.Win32NT)
        {
            config.Path = "C:\\";
        }
    }

    private Config LoadConfig()
    {
        if (!File.Exists(ConfigFile)) return new Config();
        string json = File.ReadAllText(ConfigFile);
        return JsonSerializer.Deserialize<Config>(json) ?? new Config();
    }

    private void SaveConfig()
    {
        string json = JsonSerializer.Serialize(config, Options);
        File.WriteAllText(ConfigFile, json);
    }

    private DiskUsage GetDiskUsage()
    {
        try
        {
            DriveInfo drive = new DriveInfo(config.Path);
            if (!drive.IsReady) return null;
            long total = drive.TotalSize;
            long free = drive.AvailableFreeSpace;
            long used = total - free;
            double percent = (double)used / total * 100;
            return new DiskUsage { Total = total, Used = used, Free = free, Percent = percent };
        }
        catch
        {
            return null;
        }
    }

    public void Monitor(bool once)
    {
        Console.WriteLine("💾 Disk Monitor");
        Console.WriteLine($"Monitoring: {config.Path}");
        Console.WriteLine($"Threshold: {config.Threshold}% | Interval: {config.Interval}s\n");

        bool alerted = false;
        while (true)
        {
            var usage = GetDiskUsage();
            if (usage == null)
            {
                Console.WriteLine("Error getting disk usage.");
                break;
            }
            string now = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
            double percent = usage.Percent;
            double totalGB = usage.Total / (1024.0 * 1024 * 1024);
            double usedGB = usage.Used / (1024.0 * 1024 * 1024);
            string status = percent > config.Threshold ? "🔔 ALERT" : "✅ OK";
            string color = percent > config.Threshold ? "\x1b[91m" : "\x1b[92m";
            Console.WriteLine($"[{now}] {color}{status}\x1b[0m – {percent:F1}% used ({usedGB:F1} GB / {totalGB:F1} GB)");

            if (percent > config.Threshold && !alerted)
            {
                alerted = true;
            }
            else if (percent <= config.Threshold)
            {
                alerted = false;
            }

            if (once) break;
            Thread.Sleep(config.Interval * 1000);
        }
    }

    static void Main(string[] args)
    {
        var monitor = new DiskMonitor();
        var parsed = ParseArgs(args);
        if (parsed.ContainsKey("threshold")) monitor.config.Threshold = int.Parse(parsed["threshold"]);
        if (parsed.ContainsKey("interval")) monitor.config.Interval = int.Parse(parsed["interval"]);
        if (parsed.ContainsKey("path")) monitor.config.Path = parsed["path"];
        if (parsed.ContainsKey("save"))
        {
            monitor.SaveConfig();
            Console.WriteLine("✅ Configuration saved.");
            return;
        }
        bool once = parsed.ContainsKey("once");
        monitor.Monitor(once);
    }

    static Dictionary<string, string> ParseArgs(string[] args)
    {
        var dict = new Dictionary<string, string>();
        for (int i = 0; i < args.Length; i++)
        {
            if (args[i].StartsWith("--"))
            {
                string key = args[i].Substring(2);
                if (i + 1 < args.Length && !args[i + 1].StartsWith("--"))
                    dict[key] = args[++i];
                else
                    dict[key] = "";
            }
        }
        return dict;
    }
}
