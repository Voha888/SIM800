﻿// Драйвер регистров 74HC165 с параллельным входом и последовательным выходом включенных каскадно
#ifndef __REG74HC165_H_
#define __REG74HC165_H_

#define   NUM_OF_74HC165 3 // число каскадированных регистров

#define   REG74HC165_PORT GPIOC        // порт на котором сидят регистры 74hc165

#define   CP_PIN          GPIO_Pin_14  // строб - сдвиг данных
#define   CP_PIN_UP GPIOC->ODR |=  GPIO_Pin_14
#define   CP_PIN_DOWN GPIOC->ODR   &= ~GPIO_Pin_14
#define   CP_PIN_STATE  GPIOC->IDR & GPIO_Pin_14

#define   PL_PIN          GPIO_Pin_13  //защёлкивание входов
#define   PL_PIN_UP GPIOC->ODR |=  GPIO_Pin_13
#define   PL_PIN_DOWN GPIOC->ODR   &= ~GPIO_Pin_13
#define   PL_PIN_STATE  GPIOC->IDR & GPIO_Pin_13

#define   QH_PIN          GPIO_Pin_15  //вход данных
#define   QH_PIN_UP GPIOC->ODR |=  GPIO_Pin_15
#define   QH_PIN_DOWN GPIOC->ODR   &= ~GPIO_Pin_15
#define   QH_PIN_STATE  GPIOC->IDR & GPIO_Pin_15

#define   BIT_23  (1<<22)

#define   TRUE  1
#define   FALSE 0

#define   NUM_OF_INPUT  (8*NUM_OF_74HC165) //количество входов-бит ...1 регистр 8 входов

//!!!! ДАННЫЕ КОНСТАНТЫ ПОДОБРАНЫ ПОД МЕАНДР С ЧАСТОТОЙ 1Гц И ЧАСТОТОЙ СИСТЕМНОГО ТАЙМЕРА 1КГц
#define   NOICE_DURATION            1000               // длительность шумового импульса
// если длительность импульса на чифровом входе не превышает эту величину, то
// считаем этот импульс шумом

#define   THRESHOLD_DURATION        3*NOICE_DURATION  // пороговая длительность короткого импульса в тиках системного таймера
// если по истечении этого времени после появления импулься (не шума) на входе ноль, то это
// не меандр, а постоянный сигнал
// так же эта длительность проверяется после исчезновения сигнала для надежного определения
// его отсутствия

// стадии процесса чтения бит из регистра 74hc165
// клок регистра 74hc165 срабатывает по фронту, затем необходимо немного подождать, пока он выдаст валидное состояние на выходе
enum r74hc165_stage {
    pl_low,         //после того, как прочитано NUM_BIT или при старте системы, надо защелкнуть состояние входов
    cp_low,         //CP - в нуле
    cp_high,        //CP - в единице, но на выходе QH еще нельзя читать данные
    qh_ready,       //QH - готов, читаем данные
};

// БИТОВОЕ ПОЛЕ СТАТУСА СОСТОЯНИЯ ОДНОГО ЦИФРОВОГО ВХОДА
struct diginput_status
{
    unsigned enable              : 1; // может принимать FALSE - запрещен (неучитывается при выдаче сообщений наружу), либо TRUE - разрешон
    unsigned cur_phis_state      : 1; // текущее физическое состояние: 1 - в единице (+питания)
    //                               0 - в нуле (земля)
    unsigned alarm_state         : 1; // что считать состоянием активности: 1 - +питания - активное состояние
    //                                    0 - ноль (земля) - активное состояние
    unsigned cur_log_state       : 1; // текущее логическое состояние: 1 - активное
    //                               0 - неактивное
    // получается путем учета физического состояния cur_phis_state и того, что считать активным состоянием alarm_state
    unsigned meandr_already_sent : 1; // статус отправки сообщения (напрмер SMS) о появлении активного логического состояния в виде меандра
    // если логическое состояние соответствующего входа неактивно (cur_log_state = 0), то тут тоже 0,
    // если нет, то до отправки SMS здесь 0, после отправки SMS здесь 1 (что бы SMS не отправлялись без остановки)
    unsigned const_already_sent  : 1; // статус отправки сообщения (напрмер SMS) о появлении активного логического состояния в виде постоянного сигнала
    // если логическое состояние соответствующего входа неактивно (cur_log_state = 0), то тут тоже 0,
    // если нет, то до отправки SMS здесь 0, после отправки SMS здесь 1 (что бы SMS не отправлялись без остановки)
    unsigned is_const_sig        : 1; // признак, что активное или неактивное логическое состояние держится определенный интервал времени
    unsigned is_meander          : 1; // признак, что сигнал - меандр (с частотой MEANDER_F)

};

struct digital_input
{
    struct diginput_status status;            // текущее состояние входа
    uint32_t               str_flash_page;    // страница флешь памяти где хранится строка сообщения для текущего цифрового входа (ДРУЖЕСТВЕННОЕ ИМЯ ВХОДА)
    uint8_t                flash_string_cell; // конкретная ячейка внутри страницы флеш памяти
    uint32_t               pulse_duration;    // длительность активного логического состояния на цифровом входе в тиках системного таймера
    // для определения что сигнал - это шум или меандр или постоянный уровень
    uint32_t               pause_duration;    // длительность неактивного логического состояния на цифровом входе в тиках системного таймера
    // для определения что активный сигнал пропал
};

// Структура описывает текущее состояние кофигурации цифровых входов
struct reg74hc165_current_state{
    enum r74hc165_stage  stage;                     // стадия опроса
    uint8_t              current_bit;               // текущий считываемый бит
    struct digital_input arr_res[NUM_OF_INPUT];     // массив входов содержащий всю информацию о них
};

extern struct reg74hc165_current_state reg74hc165_current_state_num1; // регистровых каскадов может быть несколько

void load_data74HC165(struct reg74hc165_current_state * current_state); // Функция вызываемая из обработчика прерывания таймера производящая побитное чтение данных с выхода 74hc165
uint8_t diginputsconfig(struct digital_input * arr_res); //функция конфигурирования цифровых входов при включении системы или переконфигурировании пользователем назначает исходные состояния и считывает

#endif
