#include "matrix_keypad.h"
#include "beeper.h"


// 定义键盘扫描函数
char keypad_scan(TIM_HandleTypeDef BEEP_TIM) {
    uint8_t row, col;
    GPIO_PinState key_val;
    GPIO_TypeDef* cols_port[3] = {COL1_GPIO_Port, COL2_GPIO_Port, COL3_GPIO_Port};  // 定义列引脚GPIO端口
    uint16_t cols_pin[3] = {COL1_Pin, COL2_Pin, COL3_Pin};  // 定义列引脚
    GPIO_TypeDef* rows_port[4] = {ROW1_GPIO_Port, ROW2_GPIO_Port, ROW3_GPIO_Port, ROW4_GPIO_Port};  // 定义行引脚GPIO端口
    uint16_t rows_pin[4] = {ROW1_Pin, ROW2_Pin, ROW3_Pin, ROW4_Pin};  // 定义行引脚
    char key_map[4][3] = {  // 定义键盘映射表
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
    };
    int key[2] = {-1, -1};

    #define ROW_SET_HIGH(ROW) HAL_GPIO_WritePin(rows_port[ROW], rows_pin[ROW], GPIO_PIN_SET)
    #define ROW_SET_LOW(ROW) HAL_GPIO_WritePin(rows_port[ROW], rows_pin[ROW], GPIO_PIN_RESET)
    #define COL_GET_VAL(COL) HAL_GPIO_ReadPin(cols_port[COL], cols_pin[COL])

    for (row = 0; row < 4; row++) {
        ROW_SET_HIGH(row);
        for (col = 0; col < 3; col++) {
            key_val = COL_GET_VAL(col);
            if(key_val == GPIO_PIN_SET) {
                HAL_Delay(10);
                key_val = COL_GET_VAL(col);
                if(key_val == GPIO_PIN_SET) {
                    key[0] = row;
                    key[1] = col;
                    while(COL_GET_VAL(col) == GPIO_PIN_SET);
                }
            }
        }
        ROW_SET_LOW(row);
    }
    if(key[0] != -1 && key[1] != -1) {
        beep_setFreq(1000, BEEP_TIM);
        HAL_Delay(100);
        beep_off(BEEP_TIM);
        return key_map[key[0]][key[1]];
    }
    return 0;
}
