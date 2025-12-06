# Running Hydrogen as a Linux Service

This guide explains how to set up Hydrogen to run automatically as a system service on Linux systems. This ensures that Hydrogen starts automatically when your system boots and can be managed using standard system tools.

## Using systemd (Modern Linux Systems)

Most modern Linux distributions use systemd for service management. This includes Ubuntu 16.04+, Debian 8+, CentOS/RHEL 7+, and Fedora.

### Creating the Service File

Create a new service file:

```bash
sudo nano /etc/systemd/system/hydrogen.service
```

Add the following content:

```ini
[Unit]
Description=Hydrogen 3D Printer Control Server
After=network.target

[Service]
Type=simple
User=hydrogen
Group=hydrogen
WorkingDirectory=/opt/hydrogen
ExecStart=/opt/hydrogen/hydrogen
Restart=always
RestartSec=5

# Security settings
NoNewPrivileges=yes
ProtectSystem=full
ProtectHome=read-only

[Install]
WantedBy=multi-user.target
```

Save the file and set proper permissions:

```bash
sudo chmod 644 /etc/systemd/system/hydrogen.service
```

### Setting Up the Service User

For security, it's recommended to run Hydrogen under a dedicated user account:

```bash
# Create hydrogen user and group
sudo useradd -r -s /bin/false hydrogen

# Create and set ownership of the application directory
sudo mkdir -p /opt/hydrogen
sudo chown -R hydrogen:hydrogen /opt/hydrogen
```

### Installing Hydrogen

Copy the Hydrogen executable and configuration:

```bash
sudo cp path/to/hydrogen /opt/hydrogen/
sudo cp path/to/hydrogen.json /opt/hydrogen/
sudo chown -R hydrogen:hydrogen /opt/hydrogen
```

Ensure proper permissions:

```bash
sudo chmod 755 /opt/hydrogen/hydrogen
sudo chmod 644 /opt/hydrogen/hydrogen.json
```

### Managing the Service

Start the service:

```bash
sudo systemctl start hydrogen
```

Enable automatic start at boot:

```bash
sudo systemctl enable hydrogen
```

Check service status:

```bash
sudo systemctl status hydrogen
```

Stop the service:

```bash
sudo systemctl stop hydrogen
```

View service logs:

```bash
sudo journalctl -u hydrogen
```

## Using init.d (Legacy Linux Systems)

For older Linux systems that don't use systemd, you can set up Hydrogen as an init.d service.

### Creating the Init Script

Create a new init script:

```bash
sudo nano /etc/init.d/hydrogen
```

Add the following content:

```bash
#!/bin/sh
### BEGIN INIT INFO
# Provides:          hydrogen
# Required-Start:    $network $remote_fs
# Required-Stop:     $network $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Hydrogen 3D Printer Control Server
# Description:       Controls the Hydrogen 3D printer server daemon
### END INIT INFO

PATH=/sbin:/usr/sbin:/bin:/usr/bin
DESC="Hydrogen 3D Printer Server"
NAME=hydrogen
DAEMON=/opt/hydrogen/hydrogen
DAEMON_ARGS=""
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME
USER=hydrogen
GROUP=hydrogen

# Exit if the package is not installed
[ -x "$DAEMON" ] || exit 0

# Load the VERBOSE setting and other rcS variables
. /lib/init/vars.sh

# Define LSB log_* functions
. /lib/lsb/init-functions

do_start() {
    start-stop-daemon --start --quiet --pidfile $PIDFILE --exec $DAEMON --chuid $USER:$GROUP --background --make-pidfile --test > /dev/null \
        || return 1
    start-stop-daemon --start --quiet --pidfile $PIDFILE --exec $DAEMON --chuid $USER:$GROUP --background --make-pidfile -- $DAEMON_ARGS \
        || return 2
}

do_stop() {
    start-stop-daemon --stop --quiet --retry=TERM/30/KILL/5 --pidfile $PIDFILE --name $NAME
    RETVAL="$?"
    [ "$RETVAL" = 2 ] && return 2
    rm -f $PIDFILE
    return "$RETVAL"
}

case "$1" in
    start)
        log_daemon_msg "Starting $DESC" "$NAME"
        do_start
        case "$?" in
            0|1) log_end_msg 0 ;;
            2) log_end_msg 1 ;;
        esac
        ;;
    stop)
        log_daemon_msg "Stopping $DESC" "$NAME"
        do_stop
        case "$?" in
            0|1) log_end_msg 0 ;;
            2) log_end_msg 1 ;;
        esac
        ;;
    restart|force-reload)
        log_daemon_msg "Restarting $DESC" "$NAME"
        do_stop
        case "$?" in
            0|1)
                do_start
                case "$?" in
                    0) log_end_msg 0 ;;
                    1) log_end_msg 1 ;; # Old process is still running
                    *) log_end_msg 1 ;; # Failed to start
                esac
                ;;
            *)
                # Failed to stop
                log_end_msg 1
                ;;
        esac
        ;;
    status)
        status_of_proc "$DAEMON" "$NAME" && exit 0 || exit $?
        ;;
    *)
        echo "Usage: $SCRIPTNAME {start|stop|restart|force-reload|status}" >&2
        exit 3
        ;;
esac

exit 0
```

Make the script executable:

```bash
sudo chmod 755 /etc/init.d/hydrogen
```

### Managing the Service (init.d)

Start the service:

```bash
sudo service hydrogen start
```

Enable automatic start at boot:

```bash
sudo update-rc.d hydrogen defaults
```

Check service status:

```bash
sudo service hydrogen status
```

Stop the service:

```bash
sudo service hydrogen stop
```

View logs:

```bash
tail -f /var/log/hydrogen.log
```

## Logging Considerations

When running as a service, Hydrogen's logs are handled differently depending on your service manager:

- **systemd**: Logs are captured by journald and can be viewed with `journalctl`
- **init.d**: Logs go to the file specified in your configuration (default: /var/log/hydrogen.log)

Ensure the log directory has appropriate permissions:

```bash
sudo mkdir -p /var/log/hydrogen
sudo chown hydrogen:hydrogen /var/log/hydrogen
```

## Security Considerations

1. **File Permissions**:
   - Configuration files should be readable only by the hydrogen user
   - The executable should be owned by root but readable by the hydrogen user
   - Log directories should be writable by the hydrogen user

2. **User Permissions**:
   - The hydrogen user should be a system user without login privileges
   - Use the principle of least privilege - only grant necessary permissions

3. **Network Security**:
   - Consider using a firewall to restrict access to Hydrogen's ports
   - If exposing to the internet, use a reverse proxy with HTTPS

## Troubleshooting

1. **Service Won't Start**:
   - Check permissions on all files and directories
   - Verify the hydrogen user exists and has correct permissions
   - Check system logs for errors: `sudo journalctl -xe`

2. **Permission Denied Errors**:
   - Verify ownership of all files: `ls -l /opt/hydrogen/`
   - Check SELinux/AppArmor if applicable
   - Verify systemd/init.d script permissions

3. **Network Issues**:
   - Check if ports are available: `sudo netstat -tulpn`
   - Verify firewall settings: `sudo iptables -L`
   - Ensure network is up before service starts

For additional help, check the Hydrogen log files or contact support with the relevant log entries and your service configuration.
