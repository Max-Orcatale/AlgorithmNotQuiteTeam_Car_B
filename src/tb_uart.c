#include "tb_uart.h"

static UART_HandleTypeDef huart3;

/* TX3-PB10, RX3-PB11 */
void usart3_init(void){
    // 使能 GPIOB 和 USART3 时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    // 配置 PB10 为 USART3_TX，PB11 为 USART3_RX
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // USART3 初始化
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        Error_Handler();
    }

}

u8 usart3_read_line(char *buf, u16 max_len)
{
    static u16 rx_index = 0;
    u8 ch = 0;

    if (buf == 0 || max_len < 2)
    {
        return 0;
    }

    /* 轮询方式每次尝试收 1 个字节，超时很短，适合主循环反复调用 */
    if (HAL_UART_Receive(&huart3, &ch, 1, 1) != HAL_OK)
    {
        return 0;
    }

    /* 过滤 CR，收到 LF 说明一整行结束 */
    if (ch == '\r')
    {
        return 0;
    }

    if (ch == '\n')
    {
        buf[rx_index] = '\0';
        rx_index = 0;
        return 1;
    }

    if (rx_index < (u16)(max_len - 1))
    {
        buf[rx_index++] = (char)ch;
    }
    else
    {
        /* 超长时丢弃本行，避免越界 */
        rx_index = 0;
    }

    return 0;
}

void usart3_send_string(const char *str)
{
    u16 len = 0;

    if (str == 0)
    {
        return;
    }

    while (str[len] != '\0')
    {
        len++;
    }

    if (len > 0)
    {
        HAL_UART_Transmit(&huart3, (u8 *)str, len, 100);
    }
}

u8 usart3_parse_pulses(const char *str, ArmPose *pose)
{
    u32 value = 0;
    u8 count = 0;
    u8 in_number = 0;

    if (str == 0 || pose == 0)
    {
        return 0;
    }

    pose->time_ms = 500;

    while (*str != '\0')
    {
        if (*str >= '0' && *str <= '9')
        {
            value = value * 10u + (u32)(*str - '0');
            in_number = 1;
        }
        else if (*str == ' ' || *str == '\t')
        {
            if (in_number)
            {
                if (count >= 4 || value > 65535u)
                {
                    return 0;
                }

                pose->pulse[count++] = (u16)value;
                value = 0;
                in_number = 0;
            }
        }
        else
        {
            return 0;
        }

        str++;
    }

    if (in_number)
    {
        if (count >= 4 || value > 65535u)
        {
            return 0;
        }

        pose->pulse[count++] = (u16)value;
    }

    return (count == 4) ? 1 : 0;
}
