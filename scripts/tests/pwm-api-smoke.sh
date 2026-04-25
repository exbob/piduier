#!/usr/bin/env bash
set -euo pipefail

base_url="${1:-http://127.0.0.1:8000}"

echo "[pwm-smoke] GET /api/pwm/channels"
status="$(curl -s -o /tmp/pwm_channels.out -w "%{http_code}" "${base_url}/api/pwm/channels")"
echo "[pwm-smoke] status=${status}"
cat /tmp/pwm_channels.out

if [[ "${status}" != "200" ]]; then
	echo "[pwm-smoke] FAIL: expected 200"
	exit 1
fi

echo "[pwm-smoke] POST /api/pwm/0/config"
status="$(curl -s -o /tmp/pwm_config.out -w "%{http_code}" \
	-H "Content-Type: application/json" \
	-X POST \
	-d '{"frequency_hz":1000,"duty_percent":50}' \
	"${base_url}/api/pwm/0/config")"
echo "[pwm-smoke] status=${status}"
cat /tmp/pwm_config.out
if [[ "${status}" != "200" ]]; then
	echo "[pwm-smoke] FAIL: config expected 200"
	exit 1
fi

echo "[pwm-smoke] POST /api/pwm/0/enable"
status="$(curl -s -o /tmp/pwm_enable.out -w "%{http_code}" \
	-H "Content-Type: application/json" \
	-X POST \
	-d '{"enable":1}' \
	"${base_url}/api/pwm/0/enable")"
echo "[pwm-smoke] status=${status}"
cat /tmp/pwm_enable.out
if [[ "${status}" != "200" ]]; then
	echo "[pwm-smoke] FAIL: enable expected 200"
	exit 1
fi

echo "[pwm-smoke] GET /api/pwm/channels (verify payload)"
status="$(curl -s -o /tmp/pwm_channels_verify.out -w "%{http_code}" "${base_url}/api/pwm/channels")"
echo "[pwm-smoke] status=${status}"
cat /tmp/pwm_channels_verify.out
if [[ "${status}" != "200" ]]; then
	echo "[pwm-smoke] FAIL: verify expected 200"
	exit 1
fi
if ! rg -q '"channel":0' /tmp/pwm_channels_verify.out; then
	echo "[pwm-smoke] FAIL: payload missing channel 0"
	exit 1
fi
if ! rg -q '"frequency_hz":' /tmp/pwm_channels_verify.out; then
	echo "[pwm-smoke] FAIL: payload missing frequency_hz"
	exit 1
fi

echo "[pwm-smoke] PASS"
