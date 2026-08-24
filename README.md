💾 Disk Monitor (Notifications) — Multi‑Language Disk Space Watcher
8 languages, one reliable disk monitor – keep track of disk usage, get alerted when storage runs low, and never run out of space unexpectedly.

✨ Features
📊 Real‑time monitoring – check disk usage periodically

🔔 Threshold alerts – get notified when usage exceeds a set percentage (e.g., 80%)

⏱️ Configurable interval – set how often to check (default: 60 seconds)

🖥️ Cross‑platform – works on Windows, macOS, and Linux

📝 Console notifications – clear alerts with color highlighting

💾 Persistent config – save thresholds and settings in a JSON file

🔄 One‑shot mode – run once and exit (useful for scripts)

🚀 Quick Start
All implementations follow the same CLI pattern:

bash
# Monitor with default settings (80% threshold, 60s interval)
<command>

# Set a custom threshold (90%) and interval (30s)
<command> --threshold 90 --interval 30

# Run once and exit
<command> --once

# Show help
<command> --help
Arguments:

--threshold <percentage> – alert threshold (default: 80)

--interval <seconds> – check interval (default: 60)

--once – run once and exit

--path <path> – specific disk path to monitor (default: / or C:\)

--help – show usage

📸 Example Output
text
💾 Disk Monitor
Monitoring: / (total: 256 GB, used: 180 GB, free: 76 GB)
Threshold: 80% | Interval: 60s

[2026-08-24 10:15:30] 🔔 ALERT: Disk usage is 85% (above threshold 80%)!
[2026-08-24 10:16:30] 🔔 ALERT: Disk usage is 87% (above threshold 80%)!
[2026-08-24 10:17:30] ✅ Disk usage is 83% (below threshold)
📁 Repository Structure
text
.
├── README.md
├── python/
│   └── disk_monitor.py
├── go/
│   └── disk_monitor.go
├── javascript/
│   └── disk_monitor.js
├── ruby/
│   └── disk_monitor.rb
├── php/
│   └── disk_monitor.php
├── java/
│   └── DiskMonitor.java
├── csharp/
│   └── DiskMonitor.cs
└── cpp/
    └── disk_monitor.cpp
