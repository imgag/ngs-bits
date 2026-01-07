# Creating and Managing systemd Service for GSvarServer

This guide explains how to create a custom service on an Ubuntu server using **systemd**, how to start and stop it, and how to inspect logs using **journalctl**.

---

## Table of Contents

* [What is systemd?](#what-is-systemd)
* [Service File Location](#service-file-location)
* [Creating a Custom Service](#creating-a-custom-service)
* [Service File Breakdown](#service-file-breakdown)
* [Reloading systemd](#reloading-systemd)
* [Starting and Stopping a Service](#starting-and-stopping-a-service)
* [Enabling and Disabling at Boot](#enabling-and-disabling-at-boot)
* [Checking Service Status](#checking-service-status)
* [Viewing Logs with journalctl](#viewing-logs-with-journalctl)
* [Common Troubleshooting](#common-troubleshooting)
* [Best Practices](#best-practices)

---

## What is systemd?

`systemd` is the init system used by modern Ubuntu versions. It manages:

* System services (daemons)
* Startup order
* Logging (via journald)

Every service is defined by a **unit file**.

---

## Service File Location

Custom services are usually created in:

```text
/etc/systemd/system/
```

Files must end with the `.service` extension.

Example:

```text
/etc/systemd/system/gsvar.service
```

---

## Creating a Custom Service

### Step 1: Create the Service File

```bash
sudo nano /etc/systemd/system/gsvar.service
```

### Step 2: Add Service Configuration

```ini
[Unit]
Description=GSvar server
After=network.target

[Service]
Type=simple
User=myuser
WorkingDirectory=/opt/gsvar
ExecStart=/opt/gsvar/GSvarServer
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Save and exit the file.

---

## Service File Breakdown

### [Unit]

* **Description**: Human-readable description
* **After**: Defines startup order dependencies

### [Service]

* **Type**:

  * `simple` – default, process runs in foreground
  * `forking` – background daemons
  * `oneshot` – short-lived tasks
* **User**: User account to run the service
* **WorkingDirectory**: Directory where the command runs
* **ExecStart**: Command to start the service
* **Restart**: Restart behavior (`on-failure`, `always`, `no`)

### [Install]

* **WantedBy**: Target to attach the service to (usually `multi-user.target`)

---

## Reloading systemd

After creating or modifying a service file, reload systemd:

```bash
sudo systemctl daemon-reexec
```

or

```bash
sudo systemctl daemon-reload
```

---

## Starting and Stopping a Service

### Start

```bash
sudo systemctl start gsvar.service
```

### Stop

```bash
sudo systemctl stop gsvar.service
```

### Restart

```bash
sudo systemctl restart gsvar.service
```

---

## Enabling and Disabling at Boot

### Enable (start on boot)

```bash
sudo systemctl enable gsvar.service
```

### Disable

```bash
sudo systemctl disable gsvar.service
```

---

## Checking Service Status

```bash
sudo systemctl status gsvar.service
```

This shows:

* Current state (active, failed, inactive)
* Exit codes
* Recent log output

---

## Viewing Logs with journalctl

### View All Logs for a Service

```bash
sudo journalctl -u gsvar.service
```

### View the Last 100 Entries

```bash
sudo journalctl -u gsvar.service -n 100
```

### Logs Since Boot

```bash
sudo journalctl -u gsvar.service -b
```

### Logs for a Time Range

```bash
sudo journalctl -u gsvar.service --since "2026-01-01 10:00" --until "2026-01-01 12:00"
```

---

## Common Troubleshooting

* **Service fails immediately**: Check `ExecStart` path and permissions
* **Permission denied**: Ensure `User` has access to files
* **Environment issues**: systemd does not load shell profiles
* **Binary not found**: Use absolute paths only

Check detailed logs:

```bash
sudo journalctl -xeu gsvar.service
```

---

## Best Practices

* Use absolute paths everywhere
* Run services as non-root users when possible
* Set `Restart=on-failure` for resilience
* Log to stdout/stderr (journald will capture it)
* Keep service files minimal and readable

---

## Quick Reference

```bash
sudo systemctl daemon-reload
sudo systemctl start gsvar
sudo systemctl stop gsvar
sudo systemctl restart gsvar
sudo systemctl status gsvar
```

--

[back to main page](index.md)
