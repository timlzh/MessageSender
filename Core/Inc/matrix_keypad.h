#include <main.h>

// 定义4x3矩阵键盘引脚
#define ROW1_Pin GPIO_PIN_8 // D9
#define ROW1_GPIO_Port GPIOA
#define ROW2_Pin GPIO_PIN_11 // D10
#define ROW2_GPIO_Port GPIOA
#define ROW3_Pin GPIO_PIN_5 // D11
#define ROW3_GPIO_Port GPIOB
#define ROW4_Pin GPIO_PIN_4 // D12
#define ROW4_GPIO_Port GPIOB
#define COL1_Pin GPIO_PIN_7 // A6
#define COL1_GPIO_Port GPIOA
#define COL2_Pin GPIO_PIN_3 // A5
#define COL2_GPIO_Port GPIOA
#define COL3_Pin GPIO_PIN_1 // A4
#define COL3_GPIO_Port GPIOA

char keypad_scan(TIM_HandleTypeDef BEEP_TIM);