// DiskMonitor.java
import java.io.*;
import java.nio.file.*;
import java.text.*;
import java.util.*;
import java.util.concurrent.*;
import com.google.gson.*;

class Config {
    int threshold = 80;
    int interval = 60;
    String path = "/";
}

public class DiskMonitor {
    private static final String CONFIG_FILE = "disk_monitor_config.json";
    private static final Gson gson = new GsonBuilder().setPrettyPrinting().create();
    private Config config;

    public DiskMonitor() {
        config = loadConfig();
        if (System.getProperty("os.name").toLowerCase().contains("win")) {
            config.path = "C:\\";
        }
    }

    private Config loadConfig() {
        try {
            Path path = Paths.get(CONFIG_FILE);
            if (Files.exists(path)) {
                String json = new String(Files.readAllBytes(path));
                return gson.fromJson(json, Config.class);
            }
        } catch (Exception e) {}
        return new Config();
    }

    private void saveConfig() {
        try {
            Files.write(Paths.get(CONFIG_FILE), gson.toJson(config).getBytes());
        } catch (Exception e) {}
    }

    private DiskUsage getDiskUsage() {
        try {
            File file = new File(config.path);
            long total = file.getTotalSpace();
            long free = file.getFreeSpace();
            long used = total - free;
            double percent = (double) used / total * 100;
            return new DiskUsage(total, used, free, percent);
        } catch (Exception e) {
            return null;
        }
    }

    static class DiskUsage {
        long total, used, free;
        double percent;
        DiskUsage(long total, long used, long free, double percent) {
            this.total = total; this.used = used; this.free = free; this.percent = percent;
        }
    }

    public void monitor(boolean once) throws InterruptedException {
        System.out.println("💾 Disk Monitor");
        System.out.printf("Monitoring: %s%n", config.path);
        System.out.printf("Threshold: %d%% | Interval: %ds%n%n", config.threshold, config.interval);

        boolean alerted = false;
        while (true) {
            DiskUsage usage = getDiskUsage();
            if (usage == null) {
                System.err.println("Error getting disk usage.");
                break;
            }
            String now = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(new Date());
            double percent = usage.percent;
            double totalGB = usage.total / (1024.0 * 1024 * 1024);
            double usedGB = usage.used / (1024.0 * 1024 * 1024);
            String status = percent > config.threshold ? "🔔 ALERT" : "✅ OK";
            String color = percent > config.threshold ? "\033[91m" : "\033[92m";
            System.out.printf("[%s] %s%s\033[0m – %.1f%% used (%.1f GB / %.1f GB)%n",
                now, color, status, percent, usedGB, totalGB);

            if (percent > config.threshold && !alerted) {
                alerted = true;
            } else if (percent <= config.threshold) {
                alerted = false;
            }

            if (once) break;
            Thread.sleep(config.interval * 1000L);
        }
    }

    public static void main(String[] args) throws Exception {
        DiskMonitor monitor = new DiskMonitor();
        Map<String, String> params = new HashMap<>();
        for (int i = 0; i < args.length; i++) {
            if (args[i].startsWith("--")) {
                String key = args[i].substring(2);
                if (i+1 < args.length && !args[i+1].startsWith("--")) {
                    params.put(key, args[++i]);
                } else {
                    params.put(key, "");
                }
            }
        }

        if (params.containsKey("threshold")) {
            monitor.config.threshold = Integer.parseInt(params.get("threshold"));
        }
        if (params.containsKey("interval")) {
            monitor.config.interval = Integer.parseInt(params.get("interval"));
        }
        if (params.containsKey("path")) {
            monitor.config.path = params.get("path");
        }
        if (params.containsKey("save")) {
            monitor.saveConfig();
            System.out.println("✅ Configuration saved.");
            return;
        }
        boolean once = params.containsKey("once");
        monitor.monitor(once);
    }
}
