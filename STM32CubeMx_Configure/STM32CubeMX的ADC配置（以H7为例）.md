# STM32CubeMX的ADC配置（以H7为例）



## 1、打开ADC通道

根据自己的IO口，到Analog中选择对应的ADCx和INx;

------

## 2、ADC时钟配置

首先到**Clock Configuration**里对ADC的时钟进行配置，通常来说为**20~40MHz**之间为好;

------

## 3、ADC进行初始化配置

<u>***打开Parameter Setting***</u>

<u>***默认配置需重点选择`Resolution`、`Conversion Data Management Mode`、`External Trigger Conversion Source`、`Rank` 等选项***</u>

### 3.1  ADCs_Conmon_Setting配置和解释

#### 3.1.1 Mode配置

**推荐设置**：Independent mode
**解释**：H7 里有多个 ADC（ADC1/2/3），可以互相“联机”做多模式采样，你现在只用一个 ADC，就保持独立模式即可。

### 3.2  ADC_Settings配置和解释

#### 3.2.1 Clock Prescaler配置

**推荐设置**：Asynchronous clock mode divided by 1（异步时钟，分频 1）
**解释**：这是 ADC 内部的时钟分频器；你现在 ADC 异步时钟已经是 25 MHz，再除以 1 就是 25 MHz；25 MHz 对 100 kSPS 来说绰绰有余，也在安全范围内。

#### 3.2.2 Resolution配置

**推荐设置**：ADC 12-bit resolution
**解释**：它决定 ADC 输出结果的位数，即从模拟电压到数字值的“精细程度”；H7独有的优化模式：ADC x-bit optimized resolution，比普通 12-bit 更快（Pipeline 优化），带硬件校准，噪声更低采样延迟更短。

#### 3.2.3 Scan Conversion Mode配置

**推荐设置**：Disabled
**解释**：扫描模式是一次触发采很多通道；如果你只采 1 个通道，关掉就行。

#### 3.2.4 Continuous Conversion Mode配置

**推荐设置**：Disabled
**解释**：开启时：ADC 一直自己循环转换，频率由转换时间决定，不好精确控制在 100 kSPS。我们打算用 定时器 TRGO 来精确触发 ADC，所以关掉连续模式。

#### 3.2.5 Discontinuous Conversion Mode配置

**推荐设置**：Disabled
**解释**：把一个“扫描序列”拆成几段用多次触发来完成。你现在既不扫描，又只采一个通道，完全用不到，关。

#### 3.2.6 End Of Conversion Selection配置

**推荐设置**：End of single conversion
**解释**：只有两个选项，End of single conversion和End of sequence of conversion，它们决定 EOC 标志/中断在什么时候置位。
**含义**：
**1）End of single conversion**：
每完成一个通道的转换，就认为一次 EOC。适用情况：单通道（Number of Conversion = 1），每次触发只转这一个通道——那就是每次都触发 EOC；多通道扫描时（Scan Enabled），每转完一个通道就触发一次 EOC/中断。若你用中断，会在“每个通道结束”都进一次中断；用 DMA 时问题不大，但标志位刷新得比较频繁。
**2）End of sequence of conversion**：
只在“整条规则序列完成后”才认为 EOC。适用情况：扫描多个通道时（例如 Rank1…Rank4），只有所有通道都转换完，才置 EOC。结果：若你用中断，每次扫描整套通道才进一次中断（中断频率降低）；DMA 一般配合这个更合理，一轮数据都准备好了才认为“完成”。
***如果开了扫描、采多个通道，考虑改成 “End of sequence of conversion” 来减少中断次数。***

#### 3.2.7 Overrun behaviour配置

**推荐设置**：Overrun data overwritten
**解释**：这个选项代表：新数据覆盖旧数据。配合 DMA 连续采样时，这样你拿到的总是最新的数据，不会因为偶尔来不及处理卡死。

#### 3.2.8 Left Bit Shift配置

**推荐设置**：No bit shift
**解释**：是否对转换结果做左移（×2ⁿ）。一般不需要，保持原始 12 位，后面你在软件里自己处理即可。

#### 3.2.9 Conversion Data Management Mode配置

**推荐设置**：DMA Circular Mode
**解释**：这个是 “转换结果往哪儿放、怎么管理”的总开关。
**含义**：
**1）Regular Conversion data stored in DR register only**：
规则转换结果只存到 DR 寄存器里；每次 ADC 转换结束，结果放到 `ADCx->DR`；想拿数据只能：CPU 轮询读 `HAL_ADC_GetValue()`，或者你自己手动给这个寄存器配 DMA（一般用不到）；适合：**采样很慢、偶尔读一下的场景**，比如几百 Hz 的温度、电位器。
**2）DFSDM Mode**：
把 ADC 结果送到 DFSDM 接口；DFSDM = Digital Filter for Sigma-Delta Modulators（数字滤波模块），用来做音频/高分辨率滤波等。选这个模式时，ADC 的数据会直接通过专门接口送给 DFSDM 过滤器，而不是简单地进 DR / DMA。适合：**需要 DFSDM 做后端数字滤波**（音频/电流采样等专业应用）。一般项目、普通电压采集用不到，也不建议随便选。
**3）DMA One Shot Mode**：
使用 DMA，但“一次性搬完就停”；ADC 每做一次“序列转换”（或连续一段），通过 DMA 把数据搬到你配置的缓冲区，**缓冲区满了就停止 DMA 请求**。想再采一批数据，要重新调用 `HAL_ADC_Start_DMA()`。用途：采一段固定长度波形：比如需要 4096 点的单次采集，然后离线处理 FFT 等。适合“**一次抓一段**”的采样，不是你现在这种“持续流式采样”。
**4）DMA Circular Mode **：
使用 DMA，并把缓冲区当成环形队列不停写；ADC 每次转换完成，DMA 自动把结果写入数组。写到数组末尾后，从头再写，**循环覆盖**。只要 ADC 和定时器在跑，**数据就源源不断进缓冲区**，CPU 只用在 DMA 半满/全满中断里处理。完美适配：单通道连续采样固定采样率。

#### 3.2.10 Low Power Auto Wait配置

**推荐设置**：Disabled
**解释**：打开后：ADC 做完一次转换，要等你把数据读走才会开始下一次转换。这样会让实际采样间隔和 CPU / DMA 的读速度强相关，没法保证稳定的 100 kSPS。所以关掉。

### 3.3  ADC_Regular_ConversionMode配置和解释

#### 3.3.1 Enable Regular Conversions配置

**推荐设置**：Enable
**解释**：允许使用“常规通道”（Regular group）进行转换。一般我们都是用 Regular group 来采数据，所以要开。

#### 3.3.2 Enable Regular Oversampling配置

**推荐设置**：Disable
**解释**：过采样是硬件多次采样再平均，提高有效分辨率。你先实现稳定的 100 kSPS，再考虑要不要过采样。关掉就简单很多。

#### 3.3.3 Oversampling Ratio配置

**推荐设置**：保持默认（比如 `1`），因为 oversampling 已经关了
**解释**：开启 Oversampling 时才生效。比如 4、8、16 表示硬件采 4/8/16 次再合并。

#### 3.3.4 Number Of Conversion配置

**推荐设置**：1
**解释**：一次“规则序列”中有几个通道要转换。你只采一个通道 → 写 1。

#### 3.3.5 External Trigger Conversion Source配置

**推荐设置**： 选一个定时器 TRGO，例如：`Regular Conversion launched by external event` → `TIM2_TRGO`（或你打算用的 TIMx_TRGO）
**解释**：这个设置决定“谁来触发 ADC 转换”。你看到的大概有几类：
**含义**：
**1）Regular Conversion launched by software**：
**由软件启动转换**。意思：不用外部触发，只有当你在代码里调用 `HAL_ADC_Start()` / `HAL_ADC_Start_DMA()` 或直接设置寄存器时，才开始转换。特点：采样频率由**你软件控制**（比如 while 里一直 `HAL_ADC_Start`），很难做到“精确 100 kHz”。你要固定 100 kSPS，不推荐用这个。
**2）Timer x Capture Compare n event（例如 Timer 1 Capture Compare 1 event）**：
对应硬件事件：**“定时器 X 的比较通道 n 事件（TIMx_CCn）”**。触发时刻：当定时器计数器 CNT 走到 CCRn（比较寄存器）设定的值，发生一次 Compare 事件；这时会发出一个硬件信号给 ADC，用来触发转换。用途：可以把 ADC 采样对齐到某个 PWM 波形的特定相位（比如电机控制里，在 PWM 中点采样电流）；或者做“可变采样时刻”的应用。对你这种“**固定周期采单通道**”的应用来说，不需要这么复杂，更适合用 Trigger Out。
**3）Timer x Trigger Out event （例如 Timer 4 Trigger Out event）**：
对应硬件：**TIMx_TRGO（Trigger Output）**。TRGO 的来源在定时器里可以配置，最常用的选择是：**Update event**（溢出/重装载时）作为 TRGO。也就是每个周期末尾发一个触发。对应到你要的功能：把 TIM4 配成 **更新频率 = 100 kHz**；再把 `Trigger Event Selection TRGO` 设为 `Update event`；然后在 ADC 的 External Trigger Conversion Source 选 **Timer 4 Trigger Out event**；于是定时器每 10 µs 发一个 TRGO → ADC 每 10 µs 转一次 → 采样率 = 100 kSPS。
**4）HRTIM1 Trigger Out event **：
高分辨率定时器（HRTIM）的触发输出。多用于复杂 PWM、电机控制、电源等场景，对采样时间有很精细的要求。对你目前“只要 100k 均匀采样”来说，完全可以不用。
**5）LPTIM 1/2 Out event **：
低功耗定时器（LPTIM）的输出事件。主要为了在 低功耗模式下 也能定期唤醒 ADC 采样。一般电池设备 / 低功耗应用才用，普通采样用普通 TIM 就行。

#### 3.3.6 External Trigger Conversion Edge配置

**推荐设置**：Trigger detection on the rising edge
**解释**：定时器 TRGO 是一个脉冲信号，这里选“上升沿触发一次转换”，就够了。

#### 3.3.7 Rank 1配置

在 Rank 1 子项里有两个非常重要的设置：
**1.Channel**
推荐设置：选你接信号的那一路，比如：`INP1` / `ADC_CHANNEL_1` 等。
解释：告诉 ADC “第一路要转换哪个引脚”。
**2.Sampling Time（采样时间）**
推荐设置： 中等偏大一点，比如：`32.5 cycles`
解释：总转换时间 ≈ (采样周期 + 固定 12.5 周期) / f_ADC；25 MHz 下：
$$
T_{conv} \approx \frac{32.5 + 12.5}{25MHz} \approx 1.8μs
$$
你的采样周期是 10 µs（= 1 / 100 kHz），1.8 µs 远小于 10 µs，完全来得及；采样时间长一些，对输入源阻抗高、信号不够“硬”的情况更友好，精度更稳。
**3.Offset Number**
推荐设置：No offset
解释：给这一路 ADC 结果套用哪一个硬件偏移寄存器（Offset）。在 STM32H7 里，每个 ADC 有几组 Offset 寄存器（比如 OFR1～OFR4），你可以在别的地方（寄存器 / HAL）给它设置一个数值 `OffsetValue`，然后 ADC 转换结果 = 原始值 - OffsetValue（硬件自动减）。

### 3.4  ADC_Injected_ConversionMode配置和解释

#### 3.4.1 Enable Injected Conversions配置

**推荐设置**：Disable
**解释**：注入通道（Injected group）是专门做“紧急临时插队采样”的另一组通道。你现在不用，全部关掉。以后要做类似“中断时多采几次”之类高级功能再开。

### 3.5  Analog Watchdog 1/2/3配置和解释

#### 3.5.1 Enable Analog WatchDogX Mode配置

**推荐设置**：全部不勾选
**解释**：Analog Watchdog 是硬件比较器，用来监控 ADC 值是否超出上下限（类似硬件“报警”）。配好阈值后，超过范围会触发中断。你现在只是正常采集波形/电压，先关掉，逻辑简单。以后要做“过压/欠压保护”再开。




