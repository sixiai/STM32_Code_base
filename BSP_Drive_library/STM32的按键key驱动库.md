

# STM32的按键key驱动库

包括功能有按键单击、双击和长按

### 1 主循环里使用

```c
static Key key1;
static volatile int counter = 0;

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM3_Init();

  Key_Init(&key1, USER_KEY_GPIO_Port, USER_KEY_Pin);
  HAL_TIM_Base_Start_IT(&htim9); // 1 ms 节拍，在回调里调用 Key_Tick1ms(&key1)
	HAL_TIM_Base_Start_IT(&htim10);
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		if(tim10_flag == 1)
		{
			tim10_flag = 0;
			if(tim10_counter%10 ==0)//100ms
			{
				HAL_GPIO_TogglePin(GPIOA, LED_R_Pin);
				key_event_t ev = Key_FetchEvent(&key1);  // 读取“当前事件”
        switch (ev) {
        case KEY_EVT_SINGLE: counter += 1; break;
        case KEY_EVT_DOUBLE: counter -= 1; break;
        case KEY_EVT_LONG:   counter += 3; break;
        default: break; // NONE
				}
			}
			if(tim10_counter > 10000)
				tim10_counter = 0;
			tim10_counter++;
		}
  }
  /* USER CODE END 3 */
}
}
```

### 2 1ms定时器里添加Key_Tick1ms(&key1);

```c
/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM9)//1000Hz
	{
		Key_Tick1ms(&key1);
	}
	if(htim->Instance == TIM10)//100Hz
	{
		tim10_flag = 1;
	}
}
/* USER CODE END 4 */
```

### 3 key.c和key.h

在key.h中可以调节 去抖时间、长按阈值、双击第二下的等待窗口以及按下有效电平

```c
/* ===================== 可调参数 ===================== */
#define KEY_DEBOUNCE_MS     20    /* 去抖时间 */
#define KEY_LONG_MS         1000   /* 长按阈值 */
#define KEY_DBL_MS          300   /* 双击第二下的等待窗口 */

/* 按下有效电平：上拉接法(按下为低) -> GPIO_PIN_RESET；下拉接法(按下为高) -> GPIO_PIN_SET */
#define KEY_ACTIVE_LEVEL    GPIO_PIN_RESET
```

key.c如下

```c
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

```

key.h如下

```c
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

```

