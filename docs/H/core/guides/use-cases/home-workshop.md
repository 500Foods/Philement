# Home Workshop Setup Guide

This guide provides detailed instructions for setting up Hydrogen in a home workshop environment with a single 3D printer.

## Basic Setup

1. **Physical Setup**
   - Place your 3D printer in a well-ventilated area
   - Ensure stable power supply
   - Connect printer via USB to your server machine
   - Verify printer is recognized (`ls /dev/ttyUSB*` or `ls /dev/ttyACM*`)

2. **Network Configuration**
   - Assign static IP to your server (recommended)
   - Configure your router's DHCP reservation
   - Default port: 5000 (configurable in hydrogen.json)

## Remote Access Setup

### Local Network Access

1. **Enable mDNS Discovery**

   ```json
   {
     "mDNS": {
       "Enabled": true,
       "ServiceName": "my-3d-printer",
       "ServiceType": "_hydrogen._tcp"
     }
   }
   ```

2. **Access Methods**
   - Direct IP: `http://192.168.1.x:5000`
   - mDNS name: `http://my-3d-printer.local:5000`

### External Access

1. **Reverse Proxy Setup (Using Nginx)**

   ```nginx
   server {
       listen 80;
       server_name printer.example.com;

       location / {
           proxy_pass http://localhost:5000;
           proxy_http_version 1.1;
           proxy_set_header Upgrade $http_upgrade;
           proxy_set_header Connection "upgrade";
           proxy_set_header Host $host;
       }
   }
   ```

2. **Security Considerations**
   - Use HTTPS (Let's Encrypt recommended)
   - Enable authentication
   - Configure firewall rules

## Backup Strategy

### Configuration Backup

1. **Essential Files**

   ```bash
   /etc/hydrogen/
   ├── hydrogen.json
   ├── ssl/
   └── uploads/
   ```

2. **Automated Backup Script**

   ```bash
   #!/bin/bash
   BACKUP_DIR="/backup/hydrogen"
   DATE=$(date +%Y%m%d)
   
   # Stop service
   systemctl stop hydrogen
   
   # Backup configuration
   tar -czf $BACKUP_DIR/config_$DATE.tar.gz /etc/hydrogen/
   
   # Start service
   systemctl start hydrogen
   ```

### Print History Backup

- Enable automatic backups in configuration:

  ```json
  {
    "Backup": {
      "Enabled": true,
      "Directory": "/backup/hydrogen",
      "Interval": 86400,
      "RetentionDays": 30
    }
  }
  ```

## Notification System

### Email Notifications

1. **Configure SMTP Settings**

   ```json
   {
     "Notifications": {
       "Email": {
         "Enabled": true,
         "SMTPServer": "smtp.gmail.com",
         "SMTPPort": 587,
         "Username": "your-email@gmail.com",
         "Password": "your-app-specific-password",
         "From": "your-email@gmail.com",
         "To": ["notification-email@example.com"]
       }
     }
   }
   ```

2. **Notification Events**
   - Print started
   - Print completed
   - Print failed
   - System errors
   - Temperature warnings

### Mobile Notifications

1. **Configure Push Notifications**

   ```json
   {
     "Notifications": {
       "Push": {
         "Enabled": true,
         "Service": "pushover",
         "APIKey": "your-api-key",
         "UserKey": "your-user-key"
       }
     }
   }
   ```

## Monitoring

### System Health Checks

1. **Enable Health Monitoring**

   ```json
   {
     "Monitoring": {
       "Enabled": true,
       "CheckInterval": 300,
       "AlertThresholds": {
         "CPUPercent": 80,
         "MemoryPercent": 85,
         "DiskPercent": 90
       }
     }
   }
   ```

2. **Monitor Key Metrics**
   - CPU usage
   - Memory utilization
   - Disk space
   - Print queue status
   - Temperature trends

### Temperature Monitoring

1. **Configure Temperature Alerts**

   ```json
   {
     "TemperatureMonitoring": {
       "Enabled": true,
       "CheckInterval": 60,
       "Thresholds": {
         "HotendMaxTemp": 260,
         "BedMaxTemp": 110,
         "AmbientMaxTemp": 35
       }
     }
   }
   ```

## Maintenance

### Regular Tasks

1. Check and update firmware
2. Verify backup integrity
3. Review log files
4. Update Hydrogen software
5. Check physical connections

### Log Rotation

Configure log rotation to manage disk space:

```json
{
  "Logging": {
    "MaxSize": "100M",
    "MaxBackups": 5,
    "MaxAge": 30,
    "Compress": true
  }
}
```

## Troubleshooting

### Common Issues

1. **Connection Drops**
   - Check USB cable quality
   - Verify power supply stability
   - Review USB power management settings

2. **Print Quality Issues**
   - Monitor temperature stability
   - Check filament moisture levels
   - Verify printer calibration

3. **Server Performance**
   - Review resource utilization
   - Check disk space
   - Monitor network bandwidth

### Getting Help

1. Check system logs
2. Review [Configuration Guide](/docs/H/core/reference/configuration.md)
3. Search existing issues
4. Join community forums
