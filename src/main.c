#include "main.h"

TIM_HandleTypeDef htim4;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);

static void Servo_SetPulseUs(uint16_t us);
static void Servo_SetAngleRelative(int16_t angle);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM4_Init();

    HAL_TIM_PWM_Start(&htim4, SERVO0_TIM_CHANNEL);

    while (1)
    {
        // 左30°
        Servo_SetAngleRelative(-30);
        HAL_Delay(1000);

        // 右30°
        Servo_SetAngleRelative(30);
        HAL_Delay(1000);
    }
}

// 时钟配置函数
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // 1. 先配置振荡器：外部高速时钟 HSE + PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    // 2. 再配置系统时钟来源和各总线分频
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK
                                | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1
                                | RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}


// GPIO 初始化函数
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. 打开 GPIOB 时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // 2. 打开 AFIO 时钟
    __HAL_RCC_AFIO_CLK_ENABLE();

    // 3. 把 PB9 配成复用推挽输出
    GPIO_InitStruct.Pin = SERVO0_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SERVO0_GPIO_PORT, &GPIO_InitStruct);
}


//计时器4初始化函数，配置为 PWM 模式，频率 50Hz（周期 20ms），初始占空比 7.5%（1500us）
//servo0 连接到 TIM4 的通道 4（PB9）,servo1 连接到 TIM4 的通道 3（PB8）
static void MX_TIM4_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM4_CLK_ENABLE();

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 72 - 1;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 20000 - 1;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_PWM_Init(&htim4);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, SERVO0_TIM_CHANNEL);
}

// 舵机控制函数，设置脉宽（单位：微秒）
static void Servo_SetPulseUs(uint16_t us)
{
    if (us < 1000) us = 1000;
    if (us > 2000) us = 2000;

    __HAL_TIM_SET_COMPARE(&htim4, SERVO0_TIM_CHANNEL, us);
}

static void Servo_SetAngleRelative(int16_t angle)
{
    if (angle < -90) angle = -90;
    if (angle > 90)  angle = 90;

    uint16_t pulse = 1500 + (angle * 500) / 90;
    Servo_SetPulseUs(pulse);
}

// 错误处理函数
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
