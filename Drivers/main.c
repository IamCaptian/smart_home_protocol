#include "STC8H.h"

// ==================== 硬件引脚定义 ====================
sbit KEY_K2_PIN = P3^5;
sbit KEY_K7_PIN = P3^2;
sbit LED_1      = P3^6;
sbit LED_2      = P3^7;
sbit LED_3      = P1^0;
sbit LED_4      = P1^1;
sbit LED_5      = P1^3;
sbit LED_6      = P1^2;
sbit SEL_PIN    = P1^4;
sbit LED_8      = P1^5;
sbit FREQ_PIN   = P1^7;
sbit K7_ON_PIN  = P3^3;
sbit K2_ON_PIN  = P3^4;

// ==================== 常量 ====================
#define K7_STATE_COUNT   3
#define K2_STATE_COUNT   4
#define PULSE_WIDTH      50
#define PULSE_INTERVAL   100

// 按键消抖: 连续检测到N次相同状态才确认
#define KEY_FILTER_CNT   20     // 滤波计数(降低阈值提高灵敏度)

// ==================== 全局变量 ====================
unsigned char K7_State = 0;
unsigned char K2_State = 0;

// 非阻塞脉冲控制
bit k7_pulse_active = 0;
unsigned int k7_pulse_count = 0;
bit k7_pulse_high = 0;    // 脉冲高电平阶段

bit k2_pulse_active = 0;
unsigned int k2_pulse_count = 0;
bit k2_pulse_high = 0;    // 脉冲高电平阶段
bit k2_double_mode = 0;   // 双脉冲模式
bit k2_second_pulse = 0;  // 第二脉冲阶段

// 频率检测常量 - 使用间隔(ms)表示，避免浮点运算
#define FREQ_LOW_MIN_MS  200    // 2Hz = 500ms, 5Hz = 200ms
#define FREQ_LOW_MAX_MS  500
#define FREQ_HIGH_MIN_MS 143    // 6Hz = 167ms, 7Hz = 143ms
#define FREQ_HIGH_MAX_MS 199
#define LED8_FLASH_TIME  167    // LED8慢闪时间(ms)，3Hz = 167ms亮/167ms灭
#define FREQ_TIMEOUT     2000   // 频率检测超时时间(ms)，超过此时间无脉冲则判定为无频率

// 系统节拍(定时器0) - 需要与实际系统时钟一致
#define MAIN_FOSC        24000000UL
#define TIMER0_1MS_RELOAD (65536UL - (MAIN_FOSC / 1000UL))

// 频率检测变量
unsigned char freq_status = 0;    // 频率状态: 0=无, 1=2-4Hz(长亮), 2=5-7Hz(闪烁)
unsigned int freq_interval = 0;   // 当前脉冲间隔(ms)

// LED8控制变量
unsigned int led8_flash_count = 0;
bit led8_flash_state = 0;

volatile bit g_tick_1ms = 0;

void Freq_Detect(void);
void Freq_Init(void);
void LED8_Control(void);

// ==================== 延时函数 ====================
void Delay_ms(unsigned int t)
{
    unsigned int i, j;
    for(i = t; i > 0; i--)
        for(j = 8000; j > 0; j--);
}

// ==================== LED控制 ====================
void LED_All_Off(void)
{
    LED_1 = 1; LED_2 = 1; LED_3 = 1;
    LED_4 = 1; LED_5 = 1; LED_6 = 1;
}

void LED_Single_On(unsigned char led_index)
{
    LED_All_Off();
    switch(led_index)
    {
        case 1: LED_1 = 0; break;
        case 2: LED_2 = 0; break;
        case 3: LED_3 = 0; break;
        case 4: LED_4 = 0; break;
        case 5: LED_5 = 0; break;
        case 6: LED_6 = 0; break;
    }
}

// ==================== 脉冲输出（非阻塞式）====================
void K7_ON_Start(void)
{
    if(!k7_pulse_active)
    {
        k7_pulse_active = 1;
        k7_pulse_count = PULSE_WIDTH;
        k7_pulse_high = 1;
        K7_ON_PIN = 1;
    }
}

void K2_ON_Start(unsigned char double_pulse)
{
    if(!k2_pulse_active)
    {
        k2_pulse_active = 1;
        k2_pulse_count = PULSE_WIDTH;
        k2_pulse_high = 1;
        k2_double_mode = double_pulse;
        k2_second_pulse = 0;
        K2_ON_PIN = 1;
    }
}

// 脉冲计时器，每1ms调用一次
void Pulse_Timer(void)
{
    // K7脉冲处理
    if(k7_pulse_active)
    {
        if(k7_pulse_count > 0)
        {
            k7_pulse_count--;
        }
        else
        {
            k7_pulse_active = 0;
            k7_pulse_high = 0;
            K7_ON_PIN = 0;
        }
    }
    
    // K2脉冲处理
    if(k2_pulse_active)
    {
        if(k2_pulse_count > 0)
        {
            k2_pulse_count--;
        }
        else
        {
            if(k2_pulse_high)
            {
                // 高电平阶段结束
                k2_pulse_high = 0;
                k2_pulse_count = PULSE_WIDTH;
                K2_ON_PIN = 0;
            }
            else
            {
                // 低电平阶段结束
                if(k2_double_mode && !k2_second_pulse)
                {
                    // 进入第二脉冲
                    k2_second_pulse = 1;
                    k2_pulse_count = PULSE_INTERVAL;
                    K2_ON_PIN = 1;
                    k2_pulse_high = 1;
                }
                else
                {
                    // 脉冲结束
                    k2_pulse_active = 0;
                    k2_pulse_high = 0;
                    K2_ON_PIN = 0;
                }
            }
        }
    }
}

// ==================== 状态机处理 ====================
void K7_Process(void)
{
    switch(K7_State)
    {
        case 0: LED_5 = 0; LED_6 = 1; SEL_PIN = 1; K7_ON_Start(); break;  // 第1次: SEL=1, 脉冲
        case 1: LED_6 = 0; LED_5 = 1; SEL_PIN = 0; break;                  // 第2次: SEL=0, 无脉冲
        case 2: LED_5 = 1; LED_6 = 1; SEL_PIN = 1; K7_ON_Start(); break;  // 第3次: SEL=1, 脉冲
    }
}

void K2_Process(void)
{
    switch(K2_State)
    {
        case 0: LED_Single_On(1); K2_ON_Start(0); break;  // 第1次: 单脉冲
        case 1: LED_Single_On(2); K2_ON_Start(1); break;  // 第2次: 双脉冲
        case 2: LED_Single_On(3); K2_ON_Start(0); break;  // 第3次: 单脉冲
        case 3: LED_Single_On(4); K2_ON_Start(0); break;  // 第4次: 单脉冲
    }
}

// ==================== 按键扫描 (带独立滤波) ====================
// K2按键扫描
unsigned char Key2_Scan(void)
{
    static unsigned char filter_cnt = 0;
    static unsigned char key_pressed = 0;
    static unsigned char key_lock = 0;
    
    if(KEY_K2_PIN == 0)
    {
        if(filter_cnt < KEY_FILTER_CNT)
            filter_cnt++;
        else if(!key_pressed)
        {
            key_pressed = 1;
            if(!key_lock)
            {
                key_lock = 1;
                return 1;
            }
        }
    }
    else
    {
        if(filter_cnt > 0)
            filter_cnt--;
        else
        {
            key_pressed = 0;
            key_lock = 0;
        }
    }
    return 0;
}

// K7按键扫描
unsigned char Key7_Scan(void)
{
    static unsigned char filter_cnt = 0;
    static unsigned char key_pressed = 0;
    static unsigned char key_lock = 0;
    
    if(KEY_K7_PIN == 0)
    {
        if(filter_cnt < KEY_FILTER_CNT)
            filter_cnt++;
        else if(!key_pressed)
        {
            key_pressed = 1;
            if(!key_lock)
            {
                key_lock = 1;
                return 1;
            }
        }
    }
    else
    {
        if(filter_cnt > 0)
            filter_cnt--;
        else
        {
            key_pressed = 0;
            key_lock = 0;
        }
    }
    return 0;
}

void SystemTick_Init(void)
{
    AUXR |= 0x80;
    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = (unsigned char)(TIMER0_1MS_RELOAD >> 8);
    TL0 = (unsigned char)(TIMER0_1MS_RELOAD & 0xFF);
    TF0 = 0;
    ET0 = 1;
    TR0 = 1;
    EA = 1;
}

void Timer0_Isr(void) interrupt 1
{
    TH0 = (unsigned char)(TIMER0_1MS_RELOAD >> 8);
    TL0 = (unsigned char)(TIMER0_1MS_RELOAD & 0xFF);

    g_tick_1ms = 1;
    Pulse_Timer();
    Freq_Detect();
    LED8_Control();
}

// ==================== 系统初始化 ====================
void System_Init(void)
{
    // 关闭看门狗(防止意外复位)
    WDT_CONTR = 0x00;
    
    // 关闭低压检测(防止电压波动导致复位)
    RSTCFG = 0x00;
    
    // 上电延时，等待电源稳定
    Delay_ms(100);
    
    // IO模式配置
    P1M1 = 0x00;
    P1M0 = 0x3F;    // P1.0-P1.5推挽输出, P1.7准双向口(频率检测)
    
    P3M1 = 0x00;
    P3M0 = 0xD8;    // P3.3/P3.4/P3.6/P3.7推挽输出, 其他准双向口
    
    // 端口初始值
    P1 = 0xFF;       // P1所有引脚高
    LED_4 = 0;       // LED4点亮(P1.1)
    SEL_PIN = 1;     // SEL上电默认高电平
    LED_8 = 1;       // LED8上电默认熄灭
    
    P3 = 0xFF;       // P3所有引脚高(按键使能上拉)
    K7_ON_PIN = 0;   // K7_ON上电默认低电平
    K2_ON_PIN = 0;   // K2_ON上电默认低电平
    
    // 状态初始化
    K7_State = 0;
    K2_State = 0;
    
    // 频率检测初始化
    Freq_Init();

    SystemTick_Init();
}

// ==================== 频率检测与LED8控制 ====================
static bit freq_last_state = 1;  // 频率引脚上次状态

void Freq_Detect(void)
{
    freq_interval++;
    
    // 检测到下降沿
    if(FREQ_PIN == 0 && freq_last_state == 1)
    {
        // 根据脉冲间隔判断频率范围(使用间隔ms直接比较，避免除法)
        // 2-4Hz 对应 250ms-500ms 间隔
        // 5-7Hz 对应 143ms-200ms 间隔
        if(freq_interval >= FREQ_LOW_MIN_MS && freq_interval <= FREQ_LOW_MAX_MS)
        {
            freq_status = 1;  // 2-4Hz: LED8长亮
        }
        else if(freq_interval >= FREQ_HIGH_MIN_MS && freq_interval <= FREQ_HIGH_MAX_MS)
        {
            freq_status = 2;  // 5-7Hz: LED8慢闪
        }
        else
        {
            freq_status = 0;
        }
        
        freq_interval = 0;  // 重置间隔计数器
    }
    freq_last_state = FREQ_PIN;
    
    // 超时检测：超过2秒无脉冲则判定为无频率(持续高电平或无信号)
    // 持续高电平：freq_interval一直增加，超过超时时间后熄灭
    if(freq_interval >= FREQ_TIMEOUT)
    {
        freq_status = 0;  // 无频率/持续高电平
        freq_interval = FREQ_TIMEOUT;  // 防止溢出
    }
}

// 频率检测初始化 - 读取实际引脚状态
void Freq_Init(void)
{
    freq_last_state = FREQ_PIN;  // 读取当前引脚状态
    freq_interval = 0;
    freq_status = 0;
}

void LED8_Control(void)
{
    switch(freq_status)
    {
        case 0:  // 无频率
            LED_8 = 1;  // LED8熄灭
            led8_flash_count = 0;
            led8_flash_state = 0;
            break;
            
        case 1:  // 2-4Hz: 长亮
            LED_8 = 0;  // LED8点亮
            led8_flash_count = 0;
            led8_flash_state = 0;
            break;
            
        case 2:  // 5-7Hz: 慢闪(3Hz)
            led8_flash_count++;
            if(led8_flash_count >= LED8_FLASH_TIME)
            {
                led8_flash_count = 0;
                led8_flash_state = !led8_flash_state;
                LED_8 = led8_flash_state;
            }
            break;
    }
}

// ==================== 主函数 ====================
void main(void)
{
    System_Init();
    
    while(1)
    {
        if(g_tick_1ms)
        {
            g_tick_1ms = 0;

            if(Key7_Scan())
            {
                K7_Process();
                K7_State = (K7_State + 1) % K7_STATE_COUNT;
            }
            
            if(Key2_Scan())
            {
                K2_Process();
                K2_State = (K2_State + 1) % K2_STATE_COUNT;
            }
        }
        
        // 喂狗(即使看门狗关闭也保留，防止ISP配置意外开启)
        WDT_CONTR |= 0x10;
    }
}
