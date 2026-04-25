#ifndef PWM_H
#define PWM_H

#include <stddef.h>

typedef struct {
	int channel;
	int gpio;
	int enabled;
	int exported;
	int frequency_hz;
	int duty_percent;
	char polarity[16];
} pwm_channel_status_t;

/*
 * Read PWM status for channels 0 and 1.
 *
 * @param out: Output array for channel status.
 * @param count: Output array size, must be >= 2.
 * @return: 0 on success, -1 on error.
 */
int pwm_get_channels_status(pwm_channel_status_t *out, size_t count);

/*
 * Check whether PWM device-tree node status is "okay".
 *
 * @return: 1 when status is okay, 0 when not enabled, -1 on read error.
 */
int pwm_is_device_tree_okay(void);

/*
 * Ensure pwm0 and pwm1 are exported under /sys/class/pwm/pwmchip0.
 *
 * @return: 0 on success, -1 on error.
 */
int pwm_ensure_channels_exported(void);

/*
 * Apply PWM frequency and duty cycle for a single channel.
 *
 * @param channel: PWM channel index, supported values are 0 and 1.
 * @param frequency_hz: Target frequency in Hz, range 1..1000000.
 * @param duty_percent: Target duty cycle in percent, range 1..99.
 * @param polarity: "normal" or "inversed".
 * @return: 0 on success, -1 on error.
 */
int pwm_apply_config(int channel, int frequency_hz, int duty_percent,
                     const char *polarity);

/*
 * Set PWM output enable state for a single channel.
 *
 * @param channel: PWM channel index, supported values are 0 and 1.
 * @param enable: 0 disables output, 1 enables output.
 * @return: 0 on success, -1 on error.
 */
int pwm_set_enable(int channel, int enable);

#endif
