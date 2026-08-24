// disk_monitor.js
#!/usr/bin/env node
const fs = require('fs');
const os = require('os');
const { program } = require('commander');

const CONFIG_FILE = 'disk_monitor_config.json';

class DiskMonitor {
    constructor() {
        this.config = this.loadConfig();
        this.threshold = this.config.threshold || 80;
        this.interval = this.config.interval || 60;
        this.path = this.config.path || (os.platform() === 'win32' ? 'C:\\' : '/');
    }

    loadConfig() {
        if (fs.existsSync(CONFIG_FILE)) {
            try {
                return JSON.parse(fs.readFileSync(CONFIG_FILE));
            } catch (e) {}
        }
        return {};
    }

    saveConfig() {
        fs.writeFileSync(CONFIG_FILE, JSON.stringify({
            threshold: this.threshold,
            interval: this.interval,
            path: this.path
        }, null, 2));
    }

    getDiskUsage() {
        return new Promise((resolve) => {
            const { exec } = require('child_process');
            const cmd = os.platform() === 'win32' ?
                `wmic logicaldisk where DeviceID="${this.path}" get Size,FreeSpace` :
                `df -k ${this.path} | awk 'NR==2{print $2,$3,$4}'`;
            exec(cmd, (err, stdout) => {
                if (err) {
                    resolve(null);
                    return;
                }
                let total, used, free;
                if (os.platform() === 'win32') {
                    const lines = stdout.split('\n').filter(l => l.trim());
                    if (lines.length < 2) { resolve(null); return; }
                    const parts = lines[1].trim().split(/\s+/);
                    total = parseInt(parts[1]) || 0;
                    free = parseInt(parts[0]) || 0;
                    used = total - free;
                } else {
                    const parts = stdout.trim().split(/\s+/);
                    total = parseInt(parts[0]) * 1024;
                    used = parseInt(parts[1]) * 1024;
                    free = parseInt(parts[2]) * 1024;
                }
                if (total === 0) { resolve(null); return; }
                const percent = (used / total) * 100;
                resolve({ total, used, free, percent });
            });
        });
    }

    async monitor(once = false) {
        console.log(`💾 Disk Monitor`);
        console.log(`Monitoring: ${this.path}`);
        console.log(`Threshold: ${this.threshold}% | Interval: ${this.interval}s\n`);

        let alerted = false;
        while (true) {
            const usage = await this.getDiskUsage();
            if (!usage) {
                console.error('Error getting disk usage.');
                break;
            }
            const now = new Date().toISOString().slice(0,19).replace('T',' ');
            const percent = usage.percent;
            const totalGB = usage.total / (1024**3);
            const usedGB = usage.used / (1024**3);
            const status = percent > this.threshold ? '🔔 ALERT' : '✅ OK';
            const color = percent > this.threshold ? '\x1b[91m' : '\x1b[92m';
            console.log(`[${now}] ${color}${status}\x1b[0m – ${percent.toFixed(1)}% used (${usedGB.toFixed(1)} GB / ${totalGB.toFixed(1)} GB)`);

            if (percent > this.threshold && !alerted) {
                alerted = true;
            } else if (percent <= this.threshold) {
                alerted = false;
            }

            if (once) break;
            await new Promise(resolve => setTimeout(resolve, this.interval * 1000));
        }
    }
}

program
    .option('--threshold <n>', 'Alert threshold percentage', parseInt)
    .option('--interval <n>', 'Check interval in seconds', parseInt)
    .option('--path <path>', 'Disk path to monitor')
    .option('--once', 'Run once and exit')
    .option('--save', 'Save current settings to config')
    .parse(process.argv);

const opts = program.opts();
const monitor = new DiskMonitor();
if (opts.threshold !== undefined) monitor.threshold = opts.threshold;
if (opts.interval !== undefined) monitor.interval = opts.interval;
if (opts.path) monitor.path = opts.path;
if (opts.save) {
    monitor.saveConfig();
    console.log('✅ Configuration saved.');
    process.exit(0);
}
monitor.monitor(opts.once).catch(console.error);
