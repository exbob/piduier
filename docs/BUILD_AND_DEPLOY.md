# Build and Deploy

This project uses two separate phases:

1. Build (local only)
2. Deploy (transfer only)

Do not combine them into one command.

## 1) Build

Build for Raspberry Pi 5 target (Release, default):

```bash
ARCH=arm64 ./build.sh
```

Debug build (DEBUG logs to stdout):

```bash
ARCH=arm64 ./build.sh debug
```

This generates under `./deploy/`:

- `./deploy/piduier`
- `./deploy/web/`
- `./deploy/zlog.conf` (zlog config; Release writes log file from env `PIDUIER_LOG_FILE`, default `./piduier.log` in cwd; override with `-l` / `PIDUIER_LOG_FILE`. Debug uses stdout.)

## 2) Deploy

Use the deployment script (deploy only, no build):

```bash
./deploy.sh
```

Default target:

- Host: `lsc@172.16.1.101`
- Path: `~/Desktop/piduier`

You can override:

```bash
TARGET=lsc@172.16.1.101 TARGET_DIR=~/Desktop/piduier ./deploy.sh
```

If SSH keys are not configured, the terminal will prompt for password interactively.

## 3) Run on target

Run from the directory that contains `zlog.conf` (same folder as `piduier` after deploy), or pass paths explicitly:

```bash
./piduier --zlog-conf /path/to/zlog.conf --log-file /path/to/piduier.log
# or: PIDUIER_ZLOG_CONF=... PIDUIER_LOG_FILE=... ./piduier
```

Example on device:

```bash
ssh lsc@172.16.1.101
cd ~/Desktop/piduier
./piduier
```
