

# STM32的按键key驱动库

包括功能有按键单击、双击和长按

使用教程，keil导入key.c和key.h文件，同时需要1ms的定时器进行按键捕获。

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

