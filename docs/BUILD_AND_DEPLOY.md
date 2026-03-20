# Build and Deploy

This project uses two separate phases:

1. Build (local only)
2. Deploy (transfer only)

Do not combine them into one command.

## 1) Build

Build for Raspberry Pi 5 target:

```bash
ARCH=arm64 ./build.sh
```

This generates:

- `./bin/piduier`
- `./bin/web/`

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

```bash
ssh lsc@172.16.1.101
cd ~/Desktop/piduier
./piduier
```
