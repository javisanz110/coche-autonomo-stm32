/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - Autonomous Vehicle Control
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "string.h"
#include "stdlib.h"

#define NUMERO_SECCIONES 10

/* Handle Definitions */
ADC_HandleTypeDef hadc;
TIM_HandleTypeDef htim2, htim3, htim4;
UART_HandleTypeDef huart1;

/* Global Variables */
uint8_t letra;
uint8_t num_letra = 0;
char orden[10];

unsigned char dist_min = 5;
unsigned char dist_max = 30;
uint32_t tiempo_limite = 10000;

enum msg_index {
    ahead, right, left, back, stop, min_update, max_update, invalid,
    auto_mode, manual_mode, manual_not_set, manual_already_set, auto_already_set
};

char modo_manual;
enum msg_index mode;
char* msgs[] = {
    "hacia delante\n", "hacia la derecha\n", "hacia la izquierda\n",
    "hacia atras\n", "parar\n", "distancia min actualizada\n",
    "distancia max actualizada\n", "comando no válido", "modo auto :ON\n",
    "modo manual: ON\n", "modo manual no establecido :(\n",
    "modo manual establecido\n", "modo automático establecido\n"
};

uint32_t valor_adc;
unsigned int veces_timer;
unsigned char leer;

enum modos_coche { AVANZAR, BUSCAR_SALIDA };
enum modos_coche modo;
char paso_busqueda_salida;
unsigned int tiempo_giro = 1100;
unsigned int veces_giro;

enum sentidos_marcha { ADELANTE, ATRAS };
enum sentidos_marcha sentido = ADELANTE;

enum estados { EMITIENDO, LEYENDO };
char estado, estado_prev;
unsigned int delay_trasero, delay_trasero_prev;
unsigned int delay_delantero, delay_delantero_prev;
unsigned int valor_delay_min, valor_delay_max;
char recibiendo;
enum estados_sonido { NO_SONAR, SONIDO_CONT, SONAR_INTERMITENTE };

/* Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);

void send_msg(enum msg_index i);
enum msg_index ejecutar_orden(char* orden);
void parar(void);
void avanzar(unsigned char velocidad);
void retroceder(unsigned char velocidad);
void girar_izq(unsigned char velocidad);
void girar_der(unsigned char velocidad);
void avanzar_izq(unsigned char velocidad);
void avanzar_der(unsigned char velocidad);
void retroceder_izq(unsigned char velocidad);
void retroceder_der(unsigned char velocidad);

void set_rangos(uint16_t valores[NUMERO_SECCIONES + 1]);
uint16_t get_posicion(uint16_t valores[NUMERO_SECCIONES + 1], uint16_t valor);
void salir_modo_busqueda(void);
void invertir_sentido_marcha(void);
void reset_auto_mode(void);

void trigger_delantero_ON(void);
void trigger_delantero_OFF(void);
void trigger_trasero_ON(void);
void trigger_trasero_OFF(void);

void sonido_continuo(void);
void no_sonar(void);
void sonido_on(void);
void sonar_intermitente(void);

void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
void EXTI0_IRQHandler(void);
void ADC1_IRQHandler(void);

/* ADC Handler */
void ADC1_IRQHandler(void) {
    if((ADC1->SR & (1 << 1)) != 0) {
        valor_adc = ADC1->DR;
    }
}

/* External Interrupt Handler */
void EXTI0_IRQHandler(void) {
    if((EXTI->PR & (1<<0)) != 0) {
        invertir_sentido_marcha();
        EXTI->PR |= (1<<0);
    }
}

/* Timer 4 Handler - System Timing */
void TIM4_IRQHandler(void) {
    if((TIM4->SR & (1<<1)) != 0) {
        if(estado == EMITIENDO) {
            estado = LEYENDO;
            TIM4->CCR1 += 50000;
        } else {
            estado = EMITIENDO;
            TIM4->CCR1 += 10;
        }
        TIM4->SR &= ~(1<<1);
    }
    if((TIM4->SR & (1<<2)) != 0) {
        TIM4->CCR2 += 1000;
        veces_timer++;
        if(modo == BUSCAR_SALIDA) veces_giro++;
        
        if(veces_timer == 2000) {
            leer = 1;
            veces_timer = 0;
        }
        if(veces_giro == tiempo_giro && modo == BUSCAR_SALIDA) {
            veces_giro = 0;
            paso_busqueda_salida++;
        }
        TIM4->SR &= ~(1<<2);
    }
    if((TIM4->SR & (1<<3)) != 0) {
        TIM4->CCR3 += 50000;
        TIM4->SR &= ~(1<<3);
    }
}

/* Motor Control Functions */
void avanzar_izq(unsigned char velocidad) {
    GPIOB->BSRR |= (1<<(12+16));
    TIM2->CCR3 = velocidad;
}

void retroceder_izq(unsigned char velocidad) {
    GPIOB->BSRR |= (1<<12);
    TIM2->CCR3 = 100 - velocidad;
}

void avanzar_der(unsigned char velocidad) {
    GPIOB->BSRR |= (1<<(13+16));
    TIM2->CCR4 = velocidad;
}

void retroceder_der(unsigned char velocidad) {
    GPIOB->BSRR |= (1<<13);
    TIM2->CCR4 = 100 - velocidad;
}

void avanzar(unsigned char velocidad) {
    avanzar_izq(velocidad);
    avanzar_der(velocidad);
}

void retroceder(unsigned char velocidad) {
    retroceder_izq(velocidad);
    retroceder_der(velocidad);
}

void girar_izq(unsigned char velocidad) {
    avanzar_der(velocidad);
    retroceder_izq(velocidad);
}

void girar_der(unsigned char velocidad) {
    avanzar_izq(velocidad);
    retroceder_der(velocidad);
}

void parar(void) {
    avanzar_izq(1);
    avanzar_der(1);
}

/* Ultrasonic Sensor Triggers */
void trigger_delantero_ON(void) {
    GPIOC->BSRR |= (1<<6);
}

void trigger_delantero_OFF(void) {
    GPIOC->BSRR |= (1<<(6+16));
}

void trigger_trasero_ON(void) {
    GPIOC->BSRR |= (1<<8);
}

void trigger_trasero_OFF(void) {
    GPIOC->BSRR |= (1<<(8+16));
}

/* Buzzer Control */
void sonar_intermitente(void) {
    TIM4->CCMR2 &= ~(1<<6);
    TIM4->CCMR2 |= (1<<5);
    TIM4->CCMR2 |= (1<<4);
    sonido_on();
}

void sonido_on(void) {
    TIM4->CCER |= (1<<8);
}

void no_sonar(void) {
    TIM4->CCER &= ~(1<<8);
}

void sonido_continuo(void) {
    TIM4->CCMR2 |= (1<<6);
    TIM4->CCMR2 &= ~(1<<5);
    TIM4->CCMR2 &= ~(1<<4);
    sonido_on();
}

/* Timer 3 Handler - Ultrasonic Echo Capture */
void TIM3_IRQHandler(void) {
    if((TIM3->SR & (1<<4)) != 0) {
        if(recibiendo == 0) {
            recibiendo = 1;
            TIM3->CCER = 0x3000;
            delay_trasero_prev = TIM3->CCR4;
        } else {
            recibiendo = 0;
            TIM3->CCER = 0x1000;
            delay_trasero = TIM3->CCR4 - delay_trasero_prev;
        }
        TIM3->SR &= ~(1<<4);
    }
    if((TIM3->SR & (1<<2)) != 0) {
        if(recibiendo == 0) {
            recibiendo = 1;
            TIM3->CCER = 0x0030;
            delay_delantero_prev = TIM3->CCR2;
        } else {
            recibiendo = 0;
            TIM3->CCER = 0x0010;
            delay_delantero = TIM3->CCR2 - delay_delantero_prev;
        }
        TIM3->SR &= ~(1<<2);
    }
}

/* Command Processing */
enum msg_index ejecutar_orden(char* orden) {
    if(strcmp(orden, "MODE_MANUAL") == 0) {
        if(modo_manual == 1) return manual_already_set;
        parar();
        no_sonar();
        modo_manual = 1;
        return manual_mode;
    }
    
    if(strcmp(orden, "MODE_AUTO") == 0) {
        if(modo_manual == 0) return auto_already_set;
        reset_auto_mode();
        return auto_mode;
    }
    
    if(modo_manual == 0) return manual_not_set;
    
    if(strncmp(orden, "MIN", 3) == 0) {
        char num[4];
        strcpy(num, orden+3);
        dist_min = strtol(num, NULL, 10);
        if(dist_min < 5 || dist_min > dist_max) return invalid;
        valor_delay_min = 2*dist_min / 0.034;
        return min_update;
    }
    
    if(strncmp(orden, "MAX", 3) == 0) {
        char num[4];
        strcpy(num, orden+3);
        dist_max = strtol(num, NULL, 10);
        if(dist_max > 30 || dist_min > dist_max) return invalid;
        valor_delay_max = 2*dist_max / 0.034;
        return max_update;
    }
    
    if(strcmp(orden, "GO") == 0) { avanzar(99); return ahead; }
    if(strcmp(orden, "BACK") == 0) { retroceder(99); return back; }
    if(strcmp(orden, "IZQ") == 0) { girar_izq(99); return left; }
    if(strcmp(orden, "DER") == 0) { girar_der(99); return right; }
    if(strcmp(orden, "STP") == 0) { parar(); return stop; }
    
    return invalid;
}

void send_msg(enum msg_index i) {
    HAL_UART_Transmit(&huart1, (unsigned char*)msgs[i], strlen(msgs[i]), tiempo_limite);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if(letra == '\n') {
        orden[num_letra-1] = '\0';
        mode = ejecutar_orden(orden);
        send_msg(mode);
        for(int i = 0; i<10; i++) orden[i] = '\0';
        num_letra = 0;
    } else {
        orden[num_letra] = letra;
        num_letra++;
    }
    HAL_UART_Receive_IT(&huart1, &letra, 1);
}

void set_rangos(uint16_t valores[NUMERO_SECCIONES + 1]) {
    uint16_t valor_max = 0x0fff;
    uint16_t step = valor_max / NUMERO_SECCIONES;
    uint16_t valor = 0;
    for(uint16_t i = 0; i < NUMERO_SECCIONES; i++) {
        valores[i] = valor;
        valor += step;
    }
    valores[NUMERO_SECCIONES] = valor_max + 1;
}

uint16_t get_posicion(uint16_t valores[NUMERO_SECCIONES + 1], uint16_t valor) {
    for(uint16_t i = 0; i < NUMERO_SECCIONES; i++) {
        if(valores[i] <= valor && valor <= valores[i+1]) return i;
    }
    return -1;
}

void salir_modo_busqueda(void) {
    modo = AVANZAR;
    paso_busqueda_salida = 0;
    veces_giro = 0;
    delay_trasero = -1;
    delay_delantero = -1;
}

void invertir_sentido_marcha(void) {
    if(sentido == ADELANTE) {
        sentido = ATRAS;
        TIM3->CCER = 0x1000;
    } else {
        sentido = ADELANTE;
        TIM3->CCER = 0x0010;
    }
}

void reset_auto_mode(void) {
    parar();
    modo_manual = 0;
    for(int i = 0; i<10; i++) orden[i] = '\0';
    
    TIM4->CR1 = 0;
    TIM4->CNT = 0;
    TIM4->CCR2 = 1000;
    TIM4->CCR1 = 10;
    TIM4->CCR3 = 50000;
    TIM4->CR1 |= (1<<0);
    TIM4->SR = 0;
    
    TIM3->CR1 = 0;
    TIM3->CNT = 0;
    TIM3->CCER = 0x0010;
    TIM3->CR1 |= 0x0001;
    TIM3->SR = 0;
    
    recibiendo = 0;
    delay_trasero = -1;
    delay_delantero = -1;
    estado_prev = -1;
    estado = EMITIENDO;
    veces_timer = 0;
    leer = 0;
    modo = AVANZAR;
    paso_busqueda_salida = 0;
    veces_giro = 0;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();
    MX_USART1_UART_Init();
    
    /* GPIO Configuration */
    GPIOA->MODER &= ~(1 << 0);
    GPIOA->MODER &= ~(1 << 1);
    SYSCFG->EXTICR[0] &= 0xFFF0;
    EXTI->IMR |= 1;
    EXTI->RTSR |= 1;
    NVIC->ISER[0] |= 1 << 6;
    
    /* Buzzer Configuration */
    GPIOB->MODER |= (1 << 17);
    GPIOB->MODER &= ~(1 << 16);
    GPIOB->AFR[1] = (2 << 0);
    
    /* Trigger Configuration */
    GPIOC->MODER &= ~(1 << 13);
    GPIOC->MODER |= (1 << 12);
    GPIOC->MODER &= ~(1 << 17);
    GPIOC->MODER |= (1 << 16);
    
    /* Echo Configuration (Timer Alternate Function) */
    GPIOC->MODER |= (1 << 19);
    GPIOC->MODER &= ~(1 << 18);
    GPIOC->AFR[0] &= ~(1 << 31);
    GPIOC->AFR[0] |= (1 << 29);
    
    GPIOC->MODER |= (1 << 19);
    GPIOC->MODER &= ~(1 << 18);
    GPIOC->AFR[1] &= ~(1 << 7);
    GPIOC->AFR[1] |= (1 << 5);
    
    /* ADC Configuration */
    GPIOA->MODER |= (1 << 5);
    GPIOA->MODER |= (1 << 4);
    ADC1->CR2 = 0;
    ADC1->CR1 = 0x00000020;
    ADC1->CR2 = 0x0000412;
    ADC1->SQR1 = 0x00000000;
    ADC1->SQR5 = 0x00000002;
    ADC1->CR2 |= (1<<0);
    while((ADC1->SR & 0x0040)==0);
    ADC1->CR2 |= 0x40000000;
    NVIC->ISER[0] |= (1<<18);
    
    /* Timer Configurations */
    TIM4->CR1 = 0;
    TIM4->PSC = 31;
    TIM4->ARR = 0xFFFF;
    TIM4->CCR2 = 1000;
    TIM4->CCR1 = 10;
    TIM4->CCR3 = 50000;
    TIM4->DIER |= (1<<3) | (1<<1) | (1<<2);
    NVIC->ISER[0] |= (1<<30);
    TIM4->CR1 |= (1<<0);
    
    TIM3->CR1 = 0;
    TIM3->PSC = 31;
    TIM3->CNT = 0;
    TIM3->ARR = 0xffff;
    TIM3->DIER |= (1<<2) | (1<<4);
    TIM3->CCMR1 = 0x0100;
    TIM3->CCMR2 = 0x0100;
    TIM3->CCER = 0x0010;
    TIM3->CR1 |= 0x0001;
    NVIC->ISER[0] |= (1 << 29);
    
    TIM2->CR1 = 0;
    TIM2->PSC = 31;
    TIM2->ARR = 99;
    TIM2->CCR3 = 1;
    TIM2->CCR4 = 1;
    TIM2->CCMR2 = 0x0000;
    TIM2->CCMR2 |= (1<<3) | (1<<6) | (1<<5);
    TIM2->CCMR2 |= (1<<11) | (1<<14) | (1<<13);
    TIM2->CCER |= (1<<8) | (1<<12);
    TIM2->CR1 |= (1<<7);
    TIM2->CR1 |= (1<<0);
    
    /* Motor Direction Pins */
    GPIOB->MODER &= ~(1<<25);
    GPIOB->MODER |= (1<<24);
    GPIOB->MODER &= ~(1<<27);
    GPIOB->MODER |= (1<<26);
    
    /* Motor PWM Pins */
    GPIOB->MODER |= (1<<21);
    GPIOB->MODER &= ~(1<<20);
    GPIOB->MODER |= (1<<23);
    GPIOB->MODER &= ~(1<<22);
    GPIOB->AFR[1] |= 0x00001100;
    
    /* Initialize Variables */
    recibiendo = 0;
    delay_trasero = -1;
    delay_delantero = -1;
    estado_prev = -1;
    estado = EMITIENDO;
    
    uint16_t valores[NUMERO_SECCIONES+1];
    set_rangos(valores);
    uint16_t posicion = 0;
    veces_timer = 0;
    leer = 0;
    
    enum estados_sonido sonido = NO_SONAR;
    enum estados_sonido sonido_previo = -1;
    char velocidad_max = 50;
    char velocidad = velocidad_max;
    
    modo = AVANZAR;
    paso_busqueda_salida = 0;
    veces_giro = 0;
    
    valor_delay_min = 294;
    valor_delay_max = 1765;
    
    modo_manual = 1;
    uint8_t* msg = (uint8_t*)"ready!\n";
    for(int i = 0; i<10; i++) orden[i] = '\0';
    
    HAL_UART_Transmit(&huart1, msg, strlen((char*)msg), tiempo_limite);
    HAL_UART_Receive_IT(&huart1, &letra, 1);
    
    while(1) {
        if(modo_manual == 0) {
            if(leer == 1) {
                leer = 0;
                posicion = get_posicion(valores, valor_adc);
                velocidad_max = 50 + posicion*6;
                if(velocidad_max > 99) velocidad_max = 99;
            }
            
            if(estado_prev != estado) {
                estado_prev = estado;
                if(estado == EMITIENDO) {
                    if(sentido == ADELANTE) trigger_delantero_ON();
                    else trigger_trasero_ON();
                } else {
                    if(sentido == ADELANTE) trigger_delantero_OFF();
                    else trigger_trasero_OFF();
                }
            }
            
            if(sonido_previo != sonido) {
                sonido_previo = sonido;
                switch(sonido) {
                    case NO_SONAR: no_sonar(); break;
                    case SONIDO_CONT: sonido_continuo(); break;
                    case SONAR_INTERMITENTE: sonar_intermitente(); break;
                }
            }
            
            if(modo == BUSCAR_SALIDA) {
                switch(paso_busqueda_salida) {
                    case 0:
                        (sentido == ADELANTE) ? (retroceder_izq(velocidad), avanzar_der(1)) : (avanzar_izq(velocidad), retroceder_der(1));
                        break;
                    case 1:
                        if((sentido == ADELANTE && delay_delantero > 1176) || (sentido == ATRAS && delay_trasero > 1176)) salir_modo_busqueda();
                        (sentido == ADELANTE) ? (avanzar_izq(velocidad), avanzar_der(1)) : (retroceder_izq(velocidad), avanzar_der(1));
                        break;
                    case 2:
                        (sentido == ADELANTE) ? (retroceder_der(velocidad), avanzar_izq(1)) : (avanzar_der(velocidad), avanzar_izq(1));
                        break;
                    case 3:
                        if((sentido == ADELANTE && delay_delantero > 1176) || (sentido == ATRAS && delay_trasero > 1176)) salir_modo_busqueda();
                        (sentido == ADELANTE) ? (avanzar_der(velocidad), avanzar_izq(1)) : (retroceder_der(velocidad), avanzar_izq(1));
                        break;
                    case 4:
                        invertir_sentido_marcha();
                        salir_modo_busqueda();
                        break;
                }
                if(modo == BUSCAR_SALIDA) continue;
            }
            
            if(delay_trasero < valor_delay_min || delay_delantero < valor_delay_min) {
                velocidad = 1;
                sonido = SONIDO_CONT;
            } else {
                sonido = SONAR_INTERMITENTE;
                if(delay_trasero < 588 || delay_delantero < 588) {
                    velocidad = 50 + (velocidad_max-50)/8;
                    if(modo == AVANZAR) {
                        velocidad = 70;
                        modo = BUSCAR_SALIDA;
                    }
                } else if(delay_trasero < 1176 || delay_delantero < 1176) {
                    velocidad = 50 + (velocidad_max-50)/4;
                } else if(delay_trasero < valor_delay_max || delay_delantero < valor_delay_max) {
                    velocidad = 50 + (velocidad_max-50)/2;
                } else {
                    velocidad = velocidad_max;
                    sonido = NO_SONAR;
                }
            }
            
            if(sentido == ADELANTE) avanzar(velocidad);
            else retroceder(velocidad);
        }
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
    RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

static void MX_ADC_Init(void) {
    hadc.Instance = ADC1;
    hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc.Init.Resolution = ADC_RESOLUTION_12B;
    hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    hadc.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
    hadc.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
    hadc.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
    hadc.Init.ContinuousConvMode = DISABLE;
    hadc.Init.NbrOfConversion = 1;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_CC3;
    hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc.Init.DMAContinuousRequests = DISABLE;
    HAL_ADC_Init(&hadc);
    
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_4;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);
}

static void MX_TIM2_Init(void) {
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 65535;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim2);
}

static void MX_TIM3_Init(void) {
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 0;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 65535;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim3);
}

static void MX_TIM4_Init(void) {
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 65535;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim4);
}

static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void Error_Handler(void) {
    __disable_irq();
    while (1);
}
