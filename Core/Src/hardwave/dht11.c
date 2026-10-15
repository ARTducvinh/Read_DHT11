#include "dht11.h"
#include <stdio.h>

/* DATA nối PA0 */
#define DHT11_PORT   GPIOA
#define DHT11_PIN    0U

#define DHT11_READ()  ((DHT11_PORT->IDR >> DHT11_PIN) & 1U)
#define DHT11_HIGH()  (DHT11_PORT->BSRR = (1U << DHT11_PIN))
#define DHT11_LOW()   (DHT11_PORT->BSRR = (1U << (DHT11_PIN + 16U)))

/* ---------- Delay µs bằng TIM2 (1 MHz, 32-bit) ---------- */
static void delay_us(uint32_t us)
{
    uint32_t start = TIM2->CNT;
    while ((uint32_t)(TIM2->CNT - start) < us);
}

/* Chờ khi chân còn ở mức 'level'. Trả về thời gian giữ mức (µs), -1 nếu timeout */
static int32_t wait_while(uint32_t level, uint32_t timeout_us)
{
    uint32_t start = TIM2->CNT;
    while (DHT11_READ() == level) {
        if ((uint32_t)(TIM2->CNT - start) > timeout_us)
            return -1;
    }
    return (int32_t)(TIM2->CNT - start);
}

void DHT11_Init(void)
{
    /* Bật bộ đếm TIM2 (MX_TIM2_Init chỉ cấu hình, chưa chạy) */
    TIM2->CR1 |= TIM_CR1_CEN;

    /* PA0: output open-drain, speed cao, pull-up */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    DHT11_PORT->MODER   &= ~(3U << (DHT11_PIN * 2U));
    DHT11_PORT->MODER   |=  (1U << (DHT11_PIN * 2U));   /* output */
    DHT11_PORT->OTYPER  |=  (1U << DHT11_PIN);          /* open-drain */
    DHT11_PORT->OSPEEDR |=  (3U << (DHT11_PIN * 2U));   /* high speed */
    DHT11_PORT->PUPDR   &= ~(3U << (DHT11_PIN * 2U));
    DHT11_PORT->PUPDR   |=  (1U << (DHT11_PIN * 2U));   /* pull-up */

    DHT11_HIGH();
    HAL_Delay(1000);   /* DHT11 cần ~1 s ổn định sau khi cấp nguồn */
}

/* ---------- Gửi Start + nhận 40 bit thô ---------- */
static DHT11_Status DHT11_ReadRaw(uint8_t buf[5])
{
    int32_t t;

    for (int i = 0; i < 5; i++) buf[i] = 0;

    /* 1. MCU gửi Start: kéo thấp >= 18 ms rồi thả dây */
    DHT11_LOW();
    HAL_Delay(20);
    DHT11_HIGH();
    delay_us(30);

    __disable_irq();

    /* 2. DHT11 phản hồi: 80 µs thấp + 80 µs cao */
    if (wait_while(1, 100) < 0) { __enable_irq(); return DHT11_ERR_NO_RESPONSE; }
    if (wait_while(0, 100) < 0) { __enable_irq(); return DHT11_ERR_NO_RESPONSE; }
    if (wait_while(1, 100) < 0) { __enable_irq(); return DHT11_ERR_NO_RESPONSE; }

    /* 3. Nhận 40 bit: ~27 µs cao = bit 0, ~70 µs cao = bit 1 */
    for (int i = 0; i < 40; i++) {
        if (wait_while(0, 100) < 0) { __enable_irq(); return DHT11_ERR_TIMEOUT; }

        t = wait_while(1, 120);
        if (t < 0) { __enable_irq(); return DHT11_ERR_TIMEOUT; }

        buf[i / 8] <<= 1;
        if (t > 40) buf[i / 8] |= 1U;
    }

    __enable_irq();
    return DHT11_OK;
}

/* ---------- Bộ giải mã: 5 byte thô -> độ ẩm / nhiệt độ ---------- */
DHT11_Status DHT11_Read(DHT11_Data *data)
{
    uint8_t buf[5];
    DHT11_Status st = DHT11_ReadRaw(buf);
    if (st != DHT11_OK) return st;

    if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
        return DHT11_ERR_CHECKSUM;

    data->humidity    = buf[0];
    data->hum_dec     = buf[1];
    data->temperature = buf[2];
    data->temp_dec    = buf[3];
    return DHT11_OK;
}

/* ---------- Đổi kết quả sang chuỗi ---------- */
void DHT11_ToString(DHT11_Status st, const DHT11_Data *d, char *out, uint32_t size)
{
    switch (st) {
    case DHT11_OK:
        snprintf(out, size, "Temp: %u.%u C, Hum: %u.%u %%\r\n",
                 d->temperature, d->temp_dec, d->humidity, d->hum_dec);
        break;
    case DHT11_ERR_NO_RESPONSE:
        snprintf(out, size, "DHT11: khong phan hoi\r\n");
        break;
    case DHT11_ERR_TIMEOUT:
        snprintf(out, size, "DHT11: loi timeout\r\n");
        break;
    case DHT11_ERR_CHECKSUM:
        snprintf(out, size, "DHT11: sai checksum\r\n");
        break;
    default:
        snprintf(out, size, "DHT11: loi khong xac dinh\r\n");
        break;
    }
}