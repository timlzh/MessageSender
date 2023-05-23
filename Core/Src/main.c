/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "IIC_OLED.h"
#include "cJSON.h"
#include "matrix_keypad.h"
#include "stdio.h"
#include "string.h"
#include "beeper.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HOME_PAGE 0
#define MENU_PAGE 1
#define MSG_SENDER_PAGE 2
#define MSG_INBOX_PAGE 3
#define MSG_OUTBOX_PAGE 4 
#define CAL_TIME_PAGE 5
#define INFO_PAGE 6
#define SET_NUMBER_PAGE 7
#define SET_TIME_PAGE 8
#define SET_SERVER_IP_PAGE 9
#define SETTING_PAGE 10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DAC_HandleTypeDef hdac1;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint8_t Uart1_RxBuff[2048] = {0};
uint8_t aRxBuffer;
uint8_t cAlmStr[] = "数据溢出(大于256)\r\n";
uint64_t Uart1_Rx_Cnt = 0;
uint8_t Uart1_RxFlag = 0;
uint16_t TIM7_Cnt1 = 1;
RTC_TimeTypeDef time = {0};
RTC_DateTypeDef date = {0};
cJSON *callBackArg;
void (*showPage)(void);
uint8_t showPageFlag = 0;
uint8_t play_ringtone_flag = 0;

int tunes[8] = {525, 589, 661, 700, 786, 882, 990, 1056};
int ringtone[3] = {330, 441, 661};

char ownID[] = "666";
char udpServerIP[] = "172.20.10.5";

char keyPad[12][10] = {
    "1,.?!@()",
    "2abcABC",
    "3defDEF",
    "4ghiGHI",
    "5jklJKL",
    "6mnoMNO",
    "7pqrsPQRS",
    "8tuvTUV",
    "9wxyzWXYZ",
    "*",
    "0 ",
    "#"};

struct messageEditor {
    char to[11];
    uint8_t toLen;
    char content[255];
    uint8_t contentLen;
    uint8_t typing;  // 0: to, 1: content
    uint8_t cursor;
    uint8_t isTyping;
    char typingChar;
    uint8_t typingCharIndex;
    uint8_t typingKeyIndex;
};

struct messageEditor msgEditor = {
    "",
    0,
    "",
    0,
    0,
    0,
    0,
    '\0',
    255};

// struct messageBox {
//     int id;
//     char from[11];
//     char to[11];
//     char time[20];
//     char content[255];
// };
// struct messageBox *inbox[100], *outbox[100];
// uint8_t inboxLen = 0, outboxLen = 0;
cJSON *msg_box, *outbox;


struct page {
    char *name;
    char items[15][16];
    uint8_t length;
    uint8_t point;
    uint8_t isMenu;
    uint8_t pageTypeID;
    uint8_t showTimeTitle;
    struct page *parent;
    struct page *children[15];
};
struct page *currentPage;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM6_Init(void);
static void MX_DAC1_Init(void);
static void MX_TIM7_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// 重写这个函数,重定向printf函数到串口
/*fputc*/
/* 重定向printf */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *stream)
#endif
PUTCHAR_PROTOTYPE {
    HAL_UART_Transmit(&huart1, &ch, 1, 1000);  // 发送串口，不同的单片机函数和串口命名不同，替换对于的函数串口名字即可实现不同库和不同单片机的重定向了
    return ch;
}

// // 重定向scanf函数到串口 意思就是说接受串口发过来的数据
// /*fgetc*/
// int fgetc(FILE *F) {
//     uint8_t ch_r = 0;
//     HAL_UART_Receive(&huart1, &ch_r, 1, 0xffff);  // 接收
//     return ch_r;
// }

struct page infoPage = {
    "Info",
    {},
    0,
    0,
    0,
    INFO_PAGE,
    1,
    NULL};

struct page calibrateTimePage = {
    "Calibrate Time",
    {},
    0,
    0,
    0,
    CAL_TIME_PAGE,
    0,
    NULL};

struct page sendMessagePage = {
    "Send Message",
    {},
    0,
    0,
    0,
    MSG_SENDER_PAGE,
    1,
    NULL,
    {}};

struct page inboxPage = {
    "Inbox",
    {},
    0,
    0,
    0,
    MSG_INBOX_PAGE,
    0,
    NULL};

struct page outboxPage = {
    "Outbox",
    {},
    0,
    0,
    0,
    MSG_OUTBOX_PAGE,
    0,
    NULL};

struct page messageHome = {
    "Message Home",
    {
        "Send Message",
        "Inbox",
        "Outbox"
    },
    3,
    0,
    1,
    MENU_PAGE,
    1,
    NULL,
    {&sendMessagePage, &inboxPage, &outboxPage}};

struct page setIDPage = {
    "Set Number",
    {},
    0,
    0,
    0,
    SET_NUMBER_PAGE,
    1,
    NULL};

struct page setTimePage = {
    "Set Time",
    {},
    0,
    0,
    0,
    SET_TIME_PAGE,
    1,
    NULL};

struct page setServerIPPage = {
    "Set Server IP",
    {},
    0,
    0,
    0,
    SET_SERVER_IP_PAGE,
    1,
    NULL};



struct page settingPage = {
    "Settings Page",
    {
        "Set ID",
        "Calibrate Time",
        // "Set Time",
        // "Set Server IP",
        
    },
    2,
    0,
    1,
    SETTING_PAGE,
    1,
    NULL,
    {&setIDPage, &calibrateTimePage, &setTimePage, &setServerIPPage}};

struct page mainMenu = {
    "Main Menu",
    {
        "Message",
        "Settings",
    },
    2,
    0,
    1,
    MENU_PAGE,
    1,
    NULL,
    {&messageHome, &settingPage}};

struct page homeScreen = {
    "Home Screen",
    {"Main Menu"},
    1,
    0,
    0,
    HOME_PAGE,
    0,
    NULL,
    {&mainMenu}};

void page_init() {
    mainMenu.parent = &homeScreen;
    messageHome.parent = settingPage.parent = &mainMenu;
    sendMessagePage.parent = inboxPage.parent = outboxPage.parent = &messageHome;
    setIDPage.parent = calibrateTimePage.parent = setTimePage.parent = setServerIPPage.parent = &settingPage;
}

void page_back() {
    if (currentPage->parent != NULL) {
        show_page(currentPage->parent);
    }
}

void show_page(struct page *page) {
    if (page == NULL) return;
    switch (page->pageTypeID) {
        case HOME_PAGE:
            show_homeScreen();
            currentPage = page;
            break;
        case MENU_PAGE:
            show_menu(page);
            currentPage = page;
            break;
        case MSG_SENDER_PAGE:
            currentPage = page;
            show_msg_sender();
            break;
        case MSG_INBOX_PAGE:
            currentPage = page;
            get_msg_inbox();
            break;
        case MSG_OUTBOX_PAGE:
            currentPage = page;
            get_msg_outbox();
            break;
        case CAL_TIME_PAGE:
            currentPage = page;
            show_cal_time();
            break;
        case SETTING_PAGE:
            currentPage = page;
            show_menu(page);
            break;
        case SET_NUMBER_PAGE:
            currentPage = page;
            show_set_id();
            break;
        case SET_TIME_PAGE:
            currentPage = page;
            show_set_time();
            break;
        case SET_SERVER_IP_PAGE:
            currentPage = page;
            // show_set_server_ip();
            break;
    }
}

/*
    Callback functions
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (Uart1_Rx_Cnt >= 2047)  // 溢出判断
    {
        Uart1_Rx_Cnt = 0;
        memset(Uart1_RxBuff, 0x00, sizeof(Uart1_RxBuff));
        HAL_UART_Transmit(&huart1, (uint8_t *)&cAlmStr, sizeof(cAlmStr), 0xFFFF);
    } else {
        Uart1_RxBuff[Uart1_Rx_Cnt++] = aRxBuffer;
        if ((Uart1_RxBuff[Uart1_Rx_Cnt - 1] == 0x0A) && (Uart1_RxBuff[Uart1_Rx_Cnt - 2] == 0x0D)) {
            Uart1_RxFlag = 1;
            char res[2048] = {0};
            if (Uart1_RxBuff[0] == '>') {
                strcpy(res, Uart1_RxBuff + 1);
            } else {
                strcpy(res, Uart1_RxBuff);
            }
            memset(Uart1_RxBuff, 0x00, sizeof(Uart1_RxBuff));
            Uart1_Rx_Cnt = 0;
            if (res[0] == '{') {
                callBackArg = cJSON_Parse(res);
                call_back();
            }
        }
    }

    HAL_UART_Receive_IT(&huart1, (uint8_t *)&aRxBuffer, 1);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM7) {
        TIM7_Cnt1++;
        if (TIM7_Cnt1 >= 30) {
            TIM7_Cnt1 = 0;
            reshow_time();
        }
    }
}

/*
    Settings
*/
void show_set_id() {
    OLED_ClearAll();
    show_title_time();
    OLED_Printf(0, 1, 8, 1, "3-digit ID:");
    OLED_Printf(0, 2, 16, 1, "___");
    char numbers[4] = "";
    int cursor = 0;
    while(1){
        char key = keypad_scan(htim2);
        switch (key){
            case '*':
                if (cursor > 0){
                    numbers[--cursor] = '\0';
                    OLED_ClearLine(2);
                    OLED_Printf(0, 2, 16, 1, numbers);
                    for(int i = cursor; i < 3; i++){
                        OLED_Printf(8 * i, 2, 16, 1, "_");
                    }
                }
                break;
            case '#':
                if(cursor < 3) break;
                strcpy(ownID, numbers);
                show_page(currentPage->parent);
                return;
            default:
                if (key == 0) break;
                if (cursor < 3){
                    numbers[cursor++] = key;
                    OLED_ClearLine(2);
                    OLED_Printf(0, 2, 16, 1, numbers);
                    for(int i = cursor; i < 3; i++){
                        OLED_Printf(8 * i, 2, 16, 1, "_");
                    }
                }
                break;
        }
    }
}

void print_setting_time(char *_time, int cursor){
    // _time: "00/00/00 00:00"
    OLED_ClearLine(2);
    int len = 14;
    for(int i = 0; i < len; i++){
        if (i == cursor){
            OLED_Printf(8 * i, 2, 16, 1, "_");
        } else {
            OLED_Printf(8 * i, 2, 16, 1, "%c", _time[i]);
        }
    }
}

void show_set_time(){
    OLED_ClearAll();
    show_title_time();
    OLED_Printf(0, 1, 8, 1, "Input Time:");
    char _time[15] = "00/00/00 00:00";
    int cursor = 0;
    print_setting_time(_time, cursor);
    while(1){
        char key = keypad_scan(htim2);
        switch (key){
            case '*':
                if (cursor > 0){
                    cursor--;
                    while(_time[cursor] < '0' || _time[cursor] > '9') cursor--;
                    _time[cursor] = '0';
                    print_setting_time(_time, cursor);
                }
                break;
            case '#':
                if(cursor < 14) break;
                RTC_TimeTypeDef sTime = {0};
                RTC_DateTypeDef sDate = {0};
                sscanf(_time, "%d/%d/%d %d:%d", &sDate.Year, &sDate.Month, &sDate.Date, &sTime.Hours, &sTime.Minutes);
                sDate.WeekDay = date.WeekDay;
                set_time(&sTime, &sDate);
                show_page(currentPage->parent);
                return;
            default:
                if (key == 0) break;
                _time[cursor] = key;
                cursor++;
                while(_time[cursor] < '0' || _time[cursor] > '9') cursor++;
                print_setting_time(_time, cursor);
                break;
        }
    }
}

/*
    Time & Date Funcs
*/
void show_time(int x, int y, int size, int mode) {
    OLED_Printf(x, y, size, mode, "%02d:%02d", time.Hours, time.Minutes);
}

void show_date(int x, int y, int size, int mode, int showWeekDay) {
    if (showWeekDay)
        OLED_Printf(x, y, size, mode, "%02d/%02d/%02d %s", date.Year, date.Month, date.Date,
                    (date.WeekDay == 1)   ? "MON"
                    : (date.WeekDay == 2) ? "TUE"
                    : (date.WeekDay == 3) ? "WED"
                    : (date.WeekDay == 4) ? "THU"
                    : (date.WeekDay == 5) ? "FRI"
                    : (date.WeekDay == 6) ? "SAT"
                                          : "SUN");
    else
        OLED_Printf(x, y, size, mode, "%02d/%02d/%02d", date.Year, date.Month, date.Date);
}

void show_title_time() {
    OLED_ClearLine(0);
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    show_time(0, 0, 8, 1);
    show_date(64, 0, 8, 1, 0);
}

void set_time(RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate) {
    HAL_RTC_SetTime(&hrtc, sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, sDate, RTC_FORMAT_BIN);

    reshow_time();
    TIM7_Cnt1 = sTime->Seconds % 31;
}

void reshow_time() {
    if (currentPage->pageTypeID == HOME_PAGE) {
        show_homeScreen();
    } else if (currentPage->showTimeTitle) {
        show_title_time();
    }
}

void calibrate_time() {
    udp_init();
    // strcpy(callBack, "set_time");
    printf("{\"func\":\"get_time\",\"clbk\":\"set_time\"}\r\n");
    HAL_Delay(1000);
}

/*
    UDP Funcs
*/
void udp_init(){
    printf("AT+CIPSTART=\"UDP\",\"%s\",6000,9000,0\r\n", udpServerIP);
    HAL_Delay(100);
    printf("AT+CIPMODE=1\r\n");
    HAL_Delay(100);
    printf("AT+CIPSEND\r\n");
    HAL_Delay(100);
}

void register_ID() {
    udp_init();
    // {"func":"reg","id":"1234567890"}
    printf("{\"func\":\"reg\",\"id\":\"%s\"}\r\n", ownID);
    HAL_Delay(500);
}

/*
    Home Screen
*/
void show_homeScreen() {
    uint8_t xtime = 44;
    uint8_t xdate = 16;
    RTC_TimeTypeDef currentTime = {0};
    RTC_DateTypeDef currentDate = {0};
    HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &currentDate, RTC_FORMAT_BIN);
    if (currentPage != &homeScreen) {
        OLED_ClearAll();
        time = currentTime;
        date = currentDate;
        show_date(xdate, 0, 8, 1, 1);
        show_time(xtime, 1, 16, 1);
        OLED_Printf(127 - 4 * 8, 7, 8, 1, "MENU");
    } else {
        if (time.Hours != currentTime.Hours || time.Minutes != currentTime.Minutes) {
            time = currentTime;
            OLED_ClearLine(1);
            OLED_ClearLine(2);
            show_time(xtime, 1, 16, 1);
        }
        if (date.Date != currentDate.Date || date.Month != currentDate.Month || date.Year != currentDate.Year) {
            date = currentDate;
            OLED_ClearLine(0);
            show_date(xdate, 0, 8, 1, 1);
        }
    }
}

/*
    Message Funcs
*/
void send_msg() {
    udp_init();
    // {"func":"send_msg", "msg":{"from":"1", "to":"2", "time":"23/04/09 19:38", "content":"Hello, world!"}}
    printf("{\"func\":\"send_msg\",\"msg\":{\"from\":\"%s\",\"to\":\"%s\",\"time\":\"%02d/%02d/%02d %02d:%02d\",\"content\":\"%s\"}}\r\n", ownID, msgEditor.to, date.Year, date.Month, date.Date, time.Hours, time.Minutes, msgEditor.content);
    HAL_Delay(500);
}

void init_msg_editor() {
    // msgEditor.to[0] = '\0';
    // msgEditor.content[0] = '\0';
    memset(msgEditor.to, 0, sizeof(msgEditor.to));
    memset(msgEditor.content, 0, sizeof(msgEditor.content));
    msgEditor.toLen = 0;
    msgEditor.contentLen = 0;
    msgEditor.typing = 0;
    msgEditor.isTyping = 0;
    msgEditor.typingChar = ' ';
    msgEditor.typingCharIndex = 0;
    msgEditor.typingKeyIndex = 0;
}

void show_cursor(uint8_t x, uint8_t y) {
    if (msgEditor.isTyping == 0) {
        OLED_Printf(x, y, 8, 0, " ");
    } else {
        OLED_Printf(x, y, 8, 0, "%c", msgEditor.typingChar);
    }
}

void show_msg_editor() {
    OLED_ClearAll();
    show_title_time();

    if (msgEditor.typing == 0) {
        uint8_t x = 0;
        OLED_Printf(x, 1, 8, 1, "To: ");
        x += 4 * 8;
        if (msgEditor.toLen == 0) {
            OLED_Printf(x, 1, 8, 0, " ");
        } else {
            OLED_Printf(x, 1, 8, 1, "%s", msgEditor.to);
            x += msgEditor.toLen * 8;
            OLED_Printf(x, 1, 8, 0, " ");
        }
        OLED_Printf(0, 2, 8, 1, "Content: %s", msgEditor.content);
    } else {
        OLED_Printf(0, 1, 8, 1, "To: %s", msgEditor.to);
        OLED_Printf(0, 2, 8, 1, "Content: ");
        uint8_t x = 0;
        uint8_t y = 3;
        if (msgEditor.contentLen == 0) {
            show_cursor(x, y);
        } else {
            OLED_Printf(x, y, 8, 1, "%s", msgEditor.content);
            x += msgEditor.contentLen * 8;
            if (x > 127 - 8) {
                x = 0;
                y++;
            }
            show_cursor(x, y);
        }
    }
}

void type(uint8_t typingKeyIndex) {
    if (msgEditor.typing == 0) {
        msgEditor.to[msgEditor.toLen++] = keyPad[typingKeyIndex][0];
    } else {
        if (msgEditor.isTyping == 1 && msgEditor.typingKeyIndex != typingKeyIndex) {
            msgEditor.isTyping = 0;
            msgEditor.content[msgEditor.contentLen++] = msgEditor.typingChar;
        }

        if (msgEditor.isTyping == 1 && msgEditor.typingKeyIndex == typingKeyIndex) {
            msgEditor.typingCharIndex++;
            if (keyPad[msgEditor.typingKeyIndex][msgEditor.typingCharIndex] == '\0') {
                msgEditor.typingCharIndex = 0;
            }
            msgEditor.typingChar = keyPad[msgEditor.typingKeyIndex][msgEditor.typingCharIndex];
        } else {
            msgEditor.isTyping = 1;
            msgEditor.typingCharIndex = 0;
            msgEditor.typingKeyIndex = typingKeyIndex;
            msgEditor.typingChar = keyPad[msgEditor.typingKeyIndex][msgEditor.typingCharIndex];
        }
    }
}

void show_msg_sender() {
    OLED_ClearAll();
    show_title_time();

    uint8_t brk_flag = 1;
    show_msg_editor();
    while (brk_flag) {
        char key = keypad_scan(htim2);
        if (key == 0) continue;
        switch (key) {
            case '1':
                type(0);
                break;
            case '2':
                type(1);
                break;
            case '3':
                type(2);
                break;
            case '4':
                type(3);
                break;
            case '5':
                type(4);
                break;
            case '6':
                type(5);
                break;
            case '7':
                type(6);
                break;
            case '8':
                type(7);
                break;
            case '9':
                type(8);
                break;
            case '*':
                if (msgEditor.typing == 0) {
                    if (msgEditor.toLen > 0) {
                        msgEditor.to[--msgEditor.toLen] = '\0';
                    }
                } else {
                    if (msgEditor.isTyping == 1) {
                        msgEditor.isTyping = 0;
                    } else {
                        if (msgEditor.contentLen > 0) {
                            msgEditor.content[--msgEditor.contentLen] = '\0';
                        } else {
                            msgEditor.typing = 0;
                        }
                    }
                }
                break;
            case '0':
                type(10);
                break;
            case '#':
                if (msgEditor.typing == 0) {
                    msgEditor.typing = 1;
                } else if (msgEditor.typing == 1 && msgEditor.isTyping == 1) {
                    msgEditor.isTyping = 0;
                    msgEditor.content[msgEditor.contentLen++] = msgEditor.typingChar;
                } else {
                    OLED_ClearAll();
                    OLED_Printf(0, 3, 16, 1, "Sending...");
                    // HAL_Delay(1000);
                    send_msg();
                    OLED_ClearAll();
                    OLED_Printf(0, 3, 16, 1, "Msg Sent!");
                    HAL_Delay(1000);
                    init_msg_editor();
                    brk_flag = 0;
                }
                break;
        }
        if (brk_flag) show_msg_editor();
    }
    page_back();
}

void get_msg_inbox(){
    OLED_ClearAll();
    OLED_Printf(0, 3, 16, 1, "Loading...");
    udp_init();
    // {"func":"get_msg_inbox", "id":"1234567890"}
    printf("{\"func\":\"get_msg_inbox\", \"id\":\"%s\", \"clbk\":\"show_msg_inbox\"}\r\n", ownID);
    HAL_Delay(500);
}

void get_msg_outbox(){
    OLED_ClearAll();
    OLED_Printf(0, 3, 16, 1, "Loading...");
    udp_init();
    // {"func":"get_msg_outbox", "id":"1234567890"}
    printf("{\"func\":\"get_msg_outbox\", \"id\":\"%s\", \"clbk\":\"show_msg_outbox\"}\r\n", ownID);
    HAL_Delay(500);
}

void show_msg(cJSON *msg){
    OLED_ClearAll();
    show_title_time();

    char *from = cJSON_GetObjectItem(msg, "from")->valuestring;
    char *to = cJSON_GetObjectItem(msg, "to")->valuestring;
    char *time = cJSON_GetObjectItem(msg, "time")->valuestring;
    char *content = cJSON_GetObjectItem(msg, "content")->valuestring;
    OLED_Printf(0, 1, 8, 1, "%s -> %s", from, to);
    OLED_Printf(0, 2, 8, 1, "%s", time);
    OLED_Printf(0, 3, 8, 1, "  %s", content);
    OLED_Printf(0, 7, 8, 1, "Reply");
}

void show_msg_box(){
    OLED_ClearAll();
    show_title_time();

    int boxLen = cJSON_GetArraySize(msg_box);
    if (boxLen == 0) {
        OLED_Printf(0, 3, 16, 1, "No Msg");
        HAL_Delay(1000);
        page_back();
        return;
    }else{
        int currentIndex = boxLen - 1;
        uint8_t brk_flag = 1;
        show_msg(cJSON_GetArrayItem(msg_box, currentIndex));
        while(brk_flag){
            char key = keypad_scan(htim2);
            if (key == 0) continue;
            switch (key) {
                case '1':
                    // Reply
                    init_msg_editor();
                    strcpy (msgEditor.to, cJSON_GetObjectItem(cJSON_GetArrayItem(msg_box, currentIndex), "from")->valuestring);
                    msgEditor.toLen = strlen(msgEditor.to);
                    brk_flag = 0;
                    show_msg_sender();
                    break;
                case '4':
                    // Previous
                    if (currentIndex > 0) {
                        currentIndex--;
                    }else{
                        currentIndex = boxLen - 1;
                    }
                    show_msg(cJSON_GetArrayItem(msg_box, currentIndex));
                    break;
                case '6':
                    // Next
                    if (currentIndex < boxLen - 1) {
                        currentIndex++;
                    }else{
                        currentIndex = 0;
                    }
                    show_msg(cJSON_GetArrayItem(msg_box, currentIndex));
                    break;
                case '*':
                    brk_flag = 0;
                    page_back();
                    break;
            }
        }
    }
}

/*
    Menu
*/
void show_menu(struct page *menu) {
    OLED_ClearAll();
    show_title_time();

    int startPoint = 0, endpoint = 5;
    if (menu->point > 5) {
        startPoint = menu->point - 5;
        endpoint = menu->point;
    }
    if (endpoint > menu->length - 1) {
        endpoint = menu->length - 1;
    }
    for (int i = startPoint; i < startPoint + 6; i++) {
        if (i == menu->point) {
            OLED_Printf(0, i - startPoint + 1, 8, 1, ">");
        }
        OLED_Printf(8, i - startPoint + 1, 8, 1, menu->items[i]);
    }
}

void show_cal_time() {
    OLED_ClearAll();
    OLED_Printf(0, 3, 16, 1, "Calibrating...");
    calibrate_time();
    HAL_Delay(2000);
    OLED_ClearAll();
    OLED_Printf(0, 0, 16, 1, "Calibrated!");
    OLED_Printf(0, 3, 8, 1, "Time: ");
    show_time(16, 4, 8, 1);
    OLED_Printf(0, 5, 8, 1, "Date: ");
    show_date(16, 6, 8, 1, 1);
    HAL_Delay(3000);

    page_back();
}

void menu_up() {
    if (currentPage->point > 0) {
        currentPage->point--;
        show_menu(currentPage);
    }
}

void menu_down() {
    if (!currentPage->isMenu) return;
    if (currentPage->point < currentPage->length - 1) {
        currentPage->point++;
        show_menu(currentPage);
    }
}

/*
    Call Backs
*/
void _set_time() {
    RTC_TimeTypeDef sTime;
    sTime.Hours = cJSON_GetObjectItem(callBackArg, "h")->valueint;
    sTime.Minutes = cJSON_GetObjectItem(callBackArg, "min")->valueint;
    sTime.Seconds = cJSON_GetObjectItem(callBackArg, "s")->valueint;

    RTC_DateTypeDef sDate;
    sDate.WeekDay = cJSON_GetObjectItem(callBackArg, "w")->valueint + 1;
    sDate.Month = cJSON_GetObjectItem(callBackArg, "m")->valueint;
    sDate.Date = cJSON_GetObjectItem(callBackArg, "d")->valueint;
    sDate.Year = cJSON_GetObjectItem(callBackArg, "y")->valueint;

    set_time(&sTime, &sDate);
}

void play_ringtone(){
    for (int i = 0; i < 3; i++){
        beep_setFreq(ringtone[i], htim2);
        HAL_Delay(150);
        beep_off(htim2);
    }
}

void _recv_msg(){
    OLED_ClearAll();
    OLED_Printf((128-8*8)/2, 3, 16, 1, "New Msg!");
    OLED_Printf(0, 7, 8, 1, "*:Back   #:Read");
    play_ringtone_flag = 1;
    infoPage.parent = currentPage;
    infoPage.children[0] = &inboxPage;
    currentPage = &infoPage;
    // while(1){
    //     char key = keypad_scan(htim2);
    //     if (key == 0) continue;
    //     if (key == '*') {
    //         show_page(currentPage);
    //         break;
    //     }else if(key == '#'){
    //         get_msg_inbox();
    //         break;
    //     }
    // }
}

void _show_msg_box(){
    memset(msg_box, NULL, sizeof(msg_box));
    msg_box = cJSON_GetObjectItem(callBackArg, "msgs");
    showPage = show_msg_box;
    showPageFlag = 1;
}

void call_back() {
    char *callBack = cJSON_GetObjectItem(callBackArg, "clbk")->valuestring;
    if (callBack == NULL) return;
    if (strcmp(callBack, "set_time") == 0) {
        _set_time();
    }else if(strcmp(callBack, "recv_msg") == 0){
        _recv_msg();
    }else if(strcmp(callBack, "show_msg_inbox") == 0){
        _show_msg_box();
    }else if(strcmp(callBack, "show_msg_outbox") == 0){
        _show_msg_box();
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_TIM6_Init();
  MX_DAC1_Init();
  MX_TIM7_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
    HAL_UART_Receive_IT(&huart1, &aRxBuffer, 1);
    __HAL_TIM_CLEAR_IT(&htim7, TIM_IT_UPDATE);
    HAL_TIM_Base_Start_IT(&htim7);
    
    page_init();
    OLED_Init();
    HAL_Delay(200);
    OLED_Printf(0, 3, 16, 1, "System Starting");
    // OLED_StartHoriRoll(OLED_ROLL_RIGHT, OLED_ROLL_3, 0, 1);
    calibrate_time();
    register_ID();
    //  OLED_Test();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    play_ringtone();
    show_page(&homeScreen);
    while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        if (showPageFlag) {
            showPage();
            showPageFlag = 0;
        }

        if (play_ringtone_flag){
            play_ringtone();
            play_ringtone_flag = 0;
        }

        char key = keypad_scan(htim2);
        switch (key) {
            case '1':
                break;
            case '2':
                menu_up();
                break;
            case '3':
                break;
            case '4':
                break;
            case '5':
                break;
            case '6':
                break;
            case '7':
                break;
            case '8':
                menu_down();
                break;
            case '9':
                break;
            case '*':
                page_back();
                break;
            case '0':
                break;
            case '#':
                show_page(currentPage->children[currentPage->point]);
                break;
        }
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */

  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x10;
  sTime.Minutes = 0x9;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_FRIDAY;
  sDate.Month = RTC_MONTH_APRIL;
  sDate.Date = 0x7;
  sDate.Year = 0x23;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 79;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 399;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 9;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 7999;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 9999;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA1 PA3 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA11 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
