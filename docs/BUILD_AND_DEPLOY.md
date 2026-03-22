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
- `./deploy/piduier.conf` (JSON application config: `http_listen`, `http_port`, and structured `zlog` including `log_file`; Release logs to the path in `zlog.log_file`, typically `./piduier.log` in cwd. Debug uses stdout for logs. No environment variables.)

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

Run from the directory that contains `piduier.conf` (same folder as `piduier` after deploy), or pass the config path:

```bash
./piduier --config /path/to/piduier.conf
# short: ./piduier -f /path/to/piduier.conf
```

Example on device:

```bash
ssh lsc@172.16.1.101
cd ~/Desktop/piduier
./piduier
```
