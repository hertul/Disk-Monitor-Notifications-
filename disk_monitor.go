// disk_monitor.go
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"runtime"
	"strconv"
	"time"
)

type Config struct {
	Threshold int    `json:"threshold"`
	Interval  int    `json:"interval"`
	Path      string `json:"path"`
}

var configFile = "disk_monitor_config.json"

func loadConfig() Config {
	var cfg Config
	cfg.Threshold = 80
	cfg.Interval = 60
	cfg.Path = "/"
	if runtime.GOOS == "windows" {
		cfg.Path = "C:\\"
	}
	data, err := os.ReadFile(configFile)
	if err != nil {
		return cfg
	}
	json.Unmarshal(data, &cfg)
	return cfg
}

func saveConfig(cfg Config) {
	data, _ := json.MarshalIndent(cfg, "", "  ")
	os.WriteFile(configFile, data, 0644)
}

func getDiskUsage(path string) (total, used, free uint64, percent float64, err error) {
	var statfs syscall.Statfs_t
	err = syscall.Statfs(path, &statfs)
	if err != nil {
		return 0, 0, 0, 0, err
	}
	total = statfs.Blocks * uint64(statfs.Bsize)
	free = statfs.Bfree * uint64(statfs.Bsize)
	used = total - free
	percent = float64(used) / float64(total) * 100
	return
}

func main() {
	var (
		threshold = flag.Int("threshold", 0, "Alert threshold percentage")
		interval  = flag.Int("interval", 0, "Check interval in seconds")
		path      = flag.String("path", "", "Disk path to monitor")
		once      = flag.Bool("once", false, "Run once and exit")
		save      = flag.Bool("save", false, "Save current settings to config")
	)
	flag.Parse()

	cfg := loadConfig()
	if *threshold > 0 {
		cfg.Threshold = *threshold
	}
	if *interval > 0 {
		cfg.Interval = *interval
	}
	if *path != "" {
		cfg.Path = *path
	}
	if *save {
		saveConfig(cfg)
		fmt.Println("✅ Configuration saved.")
		return
	}

	fmt.Printf("💾 Disk Monitor\n")
	fmt.Printf("Monitoring: %s\n", cfg.Path)
	fmt.Printf("Threshold: %d%% | Interval: %ds\n\n", cfg.Threshold, cfg.Interval)

	alerted := false
	for {
		total, used, free, percent, err := getDiskUsage(cfg.Path)
		if err != nil {
			fmt.Printf("Error: %v\n", err)
			break
		}
		now := time.Now().Format("2006-01-02 15:04:05")
		status := "🔔 ALERT"
		color := "\033[91m"
		if percent <= float64(cfg.Threshold) {
			status = "✅ OK"
			color = "\033[92m"
		}
		totalGB := float64(total) / (1024 * 1024 * 1024)
		usedGB := float64(used) / (1024 * 1024 * 1024)
		fmt.Printf("[%s] %s%s\033[0m – %.1f%% used (%.1f GB / %.1f GB)\n",
			now, color, status, percent, usedGB, totalGB)

		if percent > float64(cfg.Threshold) && !alerted {
			alerted = true
		} else if percent <= float64(cfg.Threshold) {
			alerted = false
		}

		if *once {
			break
		}
		time.Sleep(time.Duration(cfg.Interval) * time.Second)
	}
}
