/* it_persist.h - ISP 参数 INI 持久化（复刻原程序 ISP_P1 节：
 * 参数注册函数 0x40f2d0 以 (对象偏移, 类型, "参数名", "ISP_P1") 声明，
 * 参数可保存/加载到 INI。复刻用原参数名作键。 */
#ifndef IT_PERSIST_H
#define IT_PERSIST_H

#include "itypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 以原参数名写入 [ISP_P1] 节。已覆盖 ItConfig 的全部可调字段。 */
ItStatus it_config_save_ini(const ItConfig *cfg, const char *path);

/* 从 [ISP_P1] 节读取键值回填 cfg：出现的键覆盖当前值，
 * 未出现的键保持不变（与原程序增量读取语义一致）。
 * unknown_keys 非空时回填无法识别的键个数。 */
ItStatus it_config_load_ini(const char *path, ItConfig *cfg,
                            int *unknown_keys);

#ifdef __cplusplus
}
#endif
#endif /* IT_PERSIST_H */
