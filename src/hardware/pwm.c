#include "pwm.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util/log.h"

#define PWM_CHIP_PATH "/sys/class/pwm/pwmchip0"
#define PWM_CHANNEL_COUNT 2
#define PWM_DT_STATUS_PATH "/proc/device-tree/axi/pcie@1000120000/rp1/pwm@98000/status"
#define PWM_MIN_FREQUENCY_HZ 1
#define PWM_MAX_FREQUENCY_HZ 1000000

static int channel_to_gpio(int channel)
{
	if (channel == 0) {
		return 12;
	}
	if (channel == 1) {
		return 13;
	}
	return -1;
}

static int is_valid_channel(int channel)
{
	return channel >= 0 && channel < PWM_CHANNEL_COUNT;
}

static int build_channel_path(int channel, const char *leaf, char *buf,
                              size_t buf_size)
{
	if (!is_valid_channel(channel) || leaf == NULL || buf == NULL ||
	    buf_size == 0) {
		return -1;
	}
	if (snprintf(buf, buf_size, "%s/pwm%d/%s", PWM_CHIP_PATH, channel, leaf) >=
	    (int)buf_size) {
		return -1;
	}
	return 0;
}

static int write_int_file(const char *path, int value)
{
	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		return -1;
	}
	int rc = (fprintf(fp, "%d", value) > 0) ? 0 : -1;
	if (fclose(fp) != 0) {
		return -1;
	}
	return rc;
}

static int read_int_file(const char *path, int *out_value)
{
	if (out_value == NULL) {
		return -1;
	}
	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		return -1;
	}
	int value = 0;
	int rc = (fscanf(fp, "%d", &value) == 1) ? 0 : -1;
	fclose(fp);
	if (rc != 0) {
		return -1;
	}
	*out_value = value;
	return 0;
}

static int read_text_file(const char *path, char *out, size_t out_size)
{
	if (path == NULL || out == NULL || out_size < 2) {
		return -1;
	}
	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		return -1;
	}
	size_t n = fread(out, 1, out_size - 1, fp);
	fclose(fp);
	if (n == 0) {
		return -1;
	}
	out[n] = '\0';
	for (size_t i = 0; i < n; i++) {
		if (out[i] == '\n' || out[i] == '\r' || out[i] == '\0') {
			out[i] = '\0';
			break;
		}
	}
	return 0;
}

static int write_text_file(const char *path, const char *value)
{
	if (path == NULL || value == NULL || value[0] == '\0') {
		return -1;
	}
	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		return -1;
	}
	int rc = (fprintf(fp, "%s", value) > 0) ? 0 : -1;
	if (fclose(fp) != 0) {
		return -1;
	}
	return rc;
}

static int is_valid_polarity(const char *polarity)
{
	return polarity != NULL &&
	       (strcmp(polarity, "normal") == 0 ||
	        strcmp(polarity, "inversed") == 0);
}

static int ensure_exported(int channel)
{
	char pwm_dir[128];
	if (snprintf(pwm_dir, sizeof(pwm_dir), "%s/pwm%d", PWM_CHIP_PATH,
	             channel) >= (int)sizeof(pwm_dir)) {
		return -1;
	}
	if (access(pwm_dir, F_OK) == 0) {
		return 0;
	}

	char export_path[128];
	if (snprintf(export_path, sizeof(export_path), "%s/export",
	             PWM_CHIP_PATH) >= (int)sizeof(export_path)) {
		return -1;
	}
	if (write_int_file(export_path, channel) != 0) {
		LOG_ERROR("[PWM] failed to export channel %d: %s", channel,
		          strerror(errno));
		return -1;
	}

	for (int i = 0; i < 20; i++) {
		if (access(pwm_dir, F_OK) == 0) {
			return 0;
		}
		usleep(10000);
	}
	LOG_ERROR("[PWM] pwm%d path did not appear after export", channel);
	return -1;
}

static int period_to_frequency_hz(int period_ns)
{
	if (period_ns <= 0) {
		return 0;
	}
	return 1000000000 / period_ns;
}

static int duty_to_percent(int duty_ns, int period_ns)
{
	if (period_ns <= 0) {
		return 0;
	}
	if (duty_ns <= 0) {
		return 0;
	}
	if (duty_ns >= period_ns) {
		return 100;
	}
	return (int)(((int64_t)duty_ns * 100LL) / (int64_t)period_ns);
}

int pwm_get_channels_status(pwm_channel_status_t *out, size_t count)
{
	if (out == NULL || count < PWM_CHANNEL_COUNT) {
		return -1;
	}

	for (int ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
		pwm_channel_status_t *item = &out[ch];
		item->channel = ch;
		item->gpio = channel_to_gpio(ch);
		item->enabled = 0;
		item->exported = 0;
		item->frequency_hz = 0;
		item->duty_percent = 0;
		snprintf(item->polarity, sizeof(item->polarity), "%s", "normal");

		char pwm_dir[128];
		if (snprintf(pwm_dir, sizeof(pwm_dir), "%s/pwm%d", PWM_CHIP_PATH, ch) >=
		    (int)sizeof(pwm_dir)) {
			return -1;
		}
		if (access(pwm_dir, F_OK) != 0) {
			continue;
		}
		item->exported = 1;

		char enable_path[128];
		char period_path[128];
		char duty_path[128];
		char polarity_path[128];
		int enable = 0;
		int period_ns = 0;
		int duty_ns = 0;
		if (build_channel_path(ch, "enable", enable_path,
		                       sizeof(enable_path)) != 0 ||
		    build_channel_path(ch, "period", period_path,
		                       sizeof(period_path)) != 0 ||
		    build_channel_path(ch, "duty_cycle", duty_path,
		                       sizeof(duty_path)) != 0 ||
		    build_channel_path(ch, "polarity", polarity_path,
		                       sizeof(polarity_path)) != 0) {
			return -1;
		}

		(void)read_int_file(enable_path, &enable);
		(void)read_int_file(period_path, &period_ns);
		(void)read_int_file(duty_path, &duty_ns);
		(void)read_text_file(polarity_path, item->polarity,
		                     sizeof(item->polarity));

		item->enabled = (enable == 1) ? 1 : 0;
		item->frequency_hz = period_to_frequency_hz(period_ns);
		item->duty_percent = duty_to_percent(duty_ns, period_ns);
	}

	return 0;
}

int pwm_is_device_tree_okay(void)
{
	FILE *fp = fopen(PWM_DT_STATUS_PATH, "r");
	if (fp == NULL) {
		return -1;
	}

	char buf[64];
	size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
	fclose(fp);
	buf[n] = '\0';

	if (strstr(buf, "okay") != NULL) {
		return 1;
	}
	return 0;
}

int pwm_ensure_channels_exported(void)
{
	for (int channel = 0; channel < PWM_CHANNEL_COUNT; channel++) {
		if (ensure_exported(channel) != 0) {
			return -1;
		}
		char pwm_dir[128];
		if (snprintf(pwm_dir, sizeof(pwm_dir), "%s/pwm%d", PWM_CHIP_PATH,
		             channel) >= (int)sizeof(pwm_dir)) {
			return -1;
		}
		if (access(pwm_dir, F_OK) != 0) {
			LOG_ERROR("[PWM] channel %d export requested but node is still missing",
			          channel);
			return -1;
		}
	}
	return 0;
}

int pwm_apply_config(int channel, int frequency_hz, int duty_percent,
                     const char *polarity)
{
	if (!is_valid_channel(channel) ||
	    frequency_hz < PWM_MIN_FREQUENCY_HZ ||
	    frequency_hz > PWM_MAX_FREQUENCY_HZ || duty_percent < 1 ||
	    duty_percent > 99 || !is_valid_polarity(polarity)) {
		return -1;
	}
	int64_t period_ns_64 = 1000000000LL / (int64_t)frequency_hz;
	if (period_ns_64 < 1 || period_ns_64 > INT32_MAX) {
		return -1;
	}
	int period_ns = (int)period_ns_64;
	int duty_ns =
	    (int)(((int64_t)period_ns * (int64_t)duty_percent) / 100LL);

	if (ensure_exported(channel) != 0) {
		return -1;
	}

	char enable_path[128];
	char period_path[128];
	char duty_path[128];
	char polarity_path[128];
	if (build_channel_path(channel, "enable", enable_path,
	                       sizeof(enable_path)) != 0 ||
	    build_channel_path(channel, "period", period_path,
	                       sizeof(period_path)) != 0 ||
	    build_channel_path(channel, "duty_cycle", duty_path,
	                       sizeof(duty_path)) != 0 ||
	    build_channel_path(channel, "polarity", polarity_path,
	                       sizeof(polarity_path)) != 0) {
		return -1;
	}

	int original_enable = 0;
	(void)read_int_file(enable_path, &original_enable);
	if (original_enable == 1 && write_int_file(enable_path, 0) != 0) {
		return -1;
	}

	if (write_int_file(period_path, period_ns) != 0 ||
	    write_int_file(duty_path, duty_ns) != 0 ||
	    write_text_file(polarity_path, polarity) != 0) {
		if (original_enable == 1) {
			(void)write_int_file(enable_path, 1);
		}
		return -1;
	}

	if (original_enable == 1 && write_int_file(enable_path, 1) != 0) {
		return -1;
	}

	return 0;
}

int pwm_set_enable(int channel, int enable)
{
	if (!is_valid_channel(channel) || (enable != 0 && enable != 1)) {
		return -1;
	}
	if (ensure_exported(channel) != 0) {
		return -1;
	}
	char enable_path[128];
	if (build_channel_path(channel, "enable", enable_path,
	                       sizeof(enable_path)) != 0) {
		return -1;
	}
	return write_int_file(enable_path, enable);
}
