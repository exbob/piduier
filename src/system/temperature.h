#ifndef TEMPERATURE_H
#define TEMPERATURE_H

// 获取 CPU 温度（摄氏度）
// 返回温度值（摄氏度），失败返回 -1.0
double temperature_get_cpu(void);

#endif  // TEMPERATURE_H
