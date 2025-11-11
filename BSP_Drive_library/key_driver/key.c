#include "key.h"

/* 读取原始引脚电平（1=高，0=低） */
static inline uint8_t key_read_raw(Key *k)
{
	return (HAL_GPIO_ReadPin(k->port, k->pin) == GPIO_PIN_SET) ? 1u : 0u;
}

//void Key_Init(Key *k, GPIO_TypeDef *port, uint16_t pin,
//              key_cb_t single, key_cb_t dbl, key_cb_t lng, void *user)
void Key_Init(Key *k, GPIO_TypeDef *port, uint16_t pin)
{
    k->port = port; k->pin = pin;
//    k->on_single = single; k->on_double = dbl; k->on_long = lng; k->user = user;

    k->stable = key_read_raw(k);
    k->last_raw = k->stable;
    k->db_cnt = 0;

    k->press_ms = 0;
    k->gap_ms = 0;
    k->click_cnt = 0;
    k->long_fired = 0;
    k->st = KEY_S_IDLE;
		k->last_event = KEY_EVT_NONE;
}

void Key_Tick1ms(Key *k)
{
    /* ---- 简单积分去抖：原始电平保持 KEY_DEBOUNCE_MS 才认定为稳定 ---- */
    uint8_t raw = key_read_raw(k);
    if (raw != k->last_raw) {
        k->last_raw = raw;
        k->db_cnt = 0;
    } else if (k->db_cnt < KEY_DEBOUNCE_MS) {
        k->db_cnt++;
        if (k->db_cnt == KEY_DEBOUNCE_MS) {
            k->stable = raw;
        }
    }

    /* 计算当前是否“按下” */
    bool pressed = (k->stable == ((KEY_ACTIVE_LEVEL == GPIO_PIN_SET) ? 1u : 0u));

    switch (k->st) {
    case KEY_S_IDLE:
        if (pressed) {
            k->st = KEY_S_PRESSED;
            k->press_ms = 0;
            k->long_fired = 0;
        }
        break;

    case KEY_S_PRESSED:
        k->press_ms++;

        if (!pressed) {
            /* 短按释放：记录一次点击，进入等待第二击窗口 */
            if (!k->long_fired) {
                k->click_cnt++;
                k->gap_ms = 0;
                k->st = KEY_S_WAIT_SECOND;
            } else {
                /* 长按已触发，松手后直接回空闲 */
                k->st = KEY_S_IDLE;
            }
        } else {
            /* 仍在按住：检测长按 */
            if (!k->long_fired && k->press_ms >= KEY_LONG_MS) {
                k->long_fired = 1;
                if (k->last_event == KEY_EVT_NONE) k->last_event = KEY_EVT_LONG;
                /* 保持在 PRESSED，直到松手回 IDLE */
            }
        }
        break;

    case KEY_S_WAIT_SECOND:
        k->gap_ms++;
        if (pressed) {
            /* 第二次按下 —— 仍需释放后才决定是否为双击；
               若此轮演变为长按，则以长按优先 */
            k->st = KEY_S_PRESSED;
            k->press_ms = 0;
        } else if (k->gap_ms >= KEY_DBL_MS) {
            /* 超时：判定单击/双击 */
            if (k->click_cnt >= 2) {
								if (k->last_event == KEY_EVT_NONE) k->last_event = KEY_EVT_DOUBLE;
						} else if (k->click_cnt == 1) {
								if (k->last_event == KEY_EVT_NONE) k->last_event = KEY_EVT_SINGLE;
						}
            k->click_cnt = 0;
            k->st = KEY_S_IDLE;
        }
        break;
    }
}
//获取按键事件
key_event_t Key_FetchEvent(Key *k)
{
    if (!k) return KEY_EVT_NONE;
    __disable_irq();
    key_event_t ev = k->last_event;
    k->last_event = KEY_EVT_NONE;
    __enable_irq();
    return ev;
}
