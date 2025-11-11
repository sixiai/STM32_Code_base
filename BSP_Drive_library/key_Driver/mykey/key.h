#ifndef __KEY_H__
#define __KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 按需替换为你的芯片族头文件 */
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* ===================== 可调参数 ===================== */
#define KEY_DEBOUNCE_MS     20    /* 去抖时间 */
#define KEY_LONG_MS         1000   /* 长按阈值 */
#define KEY_DBL_MS          300   /* 双击第二下的等待窗口 */

/* 按下有效电平：上拉接法(按下为低) -> GPIO_PIN_RESET；下拉接法(按下为高) -> GPIO_PIN_SET */
#define KEY_ACTIVE_LEVEL    GPIO_PIN_RESET

/* ===================== 事件定义 ===================== */
typedef enum {
    KEY_EVT_NONE = 0,
    KEY_EVT_SINGLE,
    KEY_EVT_DOUBLE,
    KEY_EVT_LONG
} key_event_t;

/* ===================== 按键对象 ===================== */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;

    /* 内部状态（勿改） */
    uint8_t  stable;
    uint8_t  last_raw;
    uint16_t db_cnt;

    uint16_t press_ms;
    uint16_t gap_ms;
    uint8_t  click_cnt;
    uint8_t  long_fired;
    enum {
        KEY_S_IDLE = 0,
        KEY_S_PRESSED,
        KEY_S_WAIT_SECOND
    } st;
		volatile key_event_t last_event;
} Key;

/* ===================== API ===================== */
void Key_Init(Key *k, GPIO_TypeDef *port, uint16_t pin);
key_event_t Key_FetchEvent(Key *k);
/* 每 1 ms 调一次（放在定时器中断里） */
void Key_Tick1ms(Key *k);

#ifdef __cplusplus
}
#endif
#endif /* __KEY_H__ */


