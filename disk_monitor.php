# disk_monitor.php
#!/usr/bin/env php
<?php

define('CONFIG_FILE', 'disk_monitor_config.json');

class DiskMonitor {
    private $threshold = 80;
    private $interval = 60;
    private $path = '/';
    private $config;

    public function __construct() {
        if (DIRECTORY_SEPARATOR == '\\') {
            $this->path = 'C:\\';
        }
        $this->loadConfig();
    }

    private function loadConfig() {
        if (file_exists(CONFIG_FILE)) {
            $data = json_decode(file_get_contents(CONFIG_FILE), true);
            if ($data) {
                $this->threshold = $data['threshold'] ?? 80;
                $this->interval = $data['interval'] ?? 60;
                $this->path = $data['path'] ?? $this->path;
            }
        }
    }

    public function saveConfig() {
        file_put_contents(CONFIG_FILE, json_encode([
            'threshold' => $this->threshold,
            'interval' => $this->interval,
            'path' => $this->path
        ], JSON_PRETTY_PRINT));
    }

    public function getDiskUsage() {
        $path = $this->path;
        $total = disk_total_space($path);
        if ($total === false) return null;
        $free = disk_free_space($path);
        if ($free === false) return null;
        $used = $total - $free;
        $percent = ($used / $total) * 100;
        return [
            'total' => $total,
            'used' => $used,
            'free' => $free,
            'percent' => $percent
        ];
    }

    public function monitor($once = false) {
        echo "💾 Disk Monitor\n";
        echo "Monitoring: {$this->path}\n";
        echo "Threshold: {$this->threshold}% | Interval: {$this->interval}s\n\n";

        $alerted = false;
        while (true) {
            $usage = $this->getDiskUsage();
            if (!$usage) {
                echo "Error getting disk usage.\n";
                break;
            }
            $now = date('Y-m-d H:i:s');
            $percent = $usage['percent'];
            $totalGB = $usage['total'] / (1024**3);
            $usedGB = $usage['used'] / (1024**3);
            $status = $percent > $this->threshold ? "🔔 ALERT" : "✅ OK";
            $color = $percent > $this->threshold ? "\033[91m" : "\033[92m";
            echo "[$now] $color$status\033[0m – " . number_format($percent, 1) . "% used (" . number_format($usedGB, 1) . " GB / " . number_format($totalGB, 1) . " GB)\n";

            if ($percent > $this->threshold && !$alerted) {
                $alerted = true;
            } elseif ($percent <= $this->threshold) {
                $alerted = false;
            }

            if ($once) break;
            sleep($this->interval);
        }
    }
}

$opts = getopt("", ["threshold:", "interval:", "path:", "once", "save"]);
$monitor = new DiskMonitor();
if (isset($opts['threshold'])) $monitor->threshold = (int)$opts['threshold'];
if (isset($opts['interval'])) $monitor->interval = (int)$opts['interval'];
if (isset($opts['path'])) $monitor->path = $opts['path'];
if (isset($opts['save'])) {
    $monitor->saveConfig();
    echo "✅ Configuration saved.\n";
    exit(0);
}
$once = isset($opts['once']);
$monitor->monitor($once);
?>
