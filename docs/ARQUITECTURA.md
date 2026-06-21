# 🏗️ Arquitectura del Sistema

Documentación detallada de la arquitectura hardware y software del proyecto.

---

## 📌 Diagrama General del Sistema

```
┌─────────────────────────────────────────────────────────────┐
│                  STM32L152C Discovery                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              TIMERS (Configuración)                  │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │ TIM2: PWM Motors (CH3, CH4)                          │   │
│  │ TIM3: Captura Echo Sensores (CH2, CH4)              │   │
│  │ TIM4: Sistema Timing (CH1, CH2, CH3)                 │   │
│  └──────────────────────────────────────────────────────┘   │
│                          ▲                                   │
│                          │ Interrupciones                    │
│                          ▼                                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │         LÓGICA PRINCIPAL (main.c)                    │   │
│  │                                                      │   │
│  │ • Máquina de estados (AVANZAR/BUSCAR_SALIDA)        │   │
│  │ • Control de velocidad según distancia              │   │
│  │ • Procesamiento comandos UART/Bluetooth             │   │
│  │ • Lectura ADC (Potenciómetro)                       │   │
│  └──────────────────────────────────────────────────────┘   │
│           ▲                         ▲                        │
│           │ Órdenes                │ Sensores               │
│           ▼                         ▼                        │
└─────────────────────────────────────────────────────────────┘
        │                                      │
        │                                      │
        ├─────────┬──────────┬─────────┘       │
        │         │          │                │
        ▼         ▼          ▼                ▼
    ┌─────┐ ┌──────┐ ┌──────┐         ┌──────────────┐
    │Motor│ │Motor │ │Buzzer│         │   Sensores   │
    │ Izq │ │ Der  │ │      │         │  Ultrasónico │
    └─────┘ └──────┘ └──────┘         │              │
                                       └──────────────┘
                                            │
                                      ┌─────┴──────┐
                                      ▼            ▼
                                   Adelante     Atrás
```

---

## 🔌 Mapeo de Pines

### GPIO - Entradas/Salidas Digitales

| Puerto | Pin | Función | Configuración | Propósito |
|--------|-----|---------|---------------|-----------|
| **PA** | PA0 | Botón cambio dirección | Digital Input + EXTI | Invertir sentido marcha |
| **PA** | PA2 | Potenciómetro | Analog Input | Lectura ADC (velocidad máx) |
| **PB** | PB8 | Zumbador | AF (PWM) | TIM4_CH3 |
| **PB** | PB10 | PWM Motor IZQ | AF (PWM) | TIM2_CH3 |
| **PB** | PB11 | PWM Motor DER | AF (PWM) | TIM2_CH4 |
| **PB** | PB12 | Dir Motor IZQ | Digital Output | Control dirección |
| **PB** | PB13 | Dir Motor DER | Digital Output | Control dirección |
| **PC** | PC6 | Trigger US Delantero | Digital Output | HC-SR04 adelante |
| **PC** | PC7 | Echo US Delantero | AF (TIM3_CH2) | TIM3 captura adelante |
| **PC** | PC8 | Trigger US Trasero | Digital Output | HC-SR04 trasero |
| **PC** | PC9 | Echo US Trasero | AF (TIM3_CH4) | TIM3 captura trasero |

### UART
| Pin | Función | Configuración |
|-----|---------|---------------|
| PA9 | TX (Transmit) | Bluetooth TX |
| PA10 | RX (Receive) | Bluetooth RX |

---

## ⏱️ Configuración de Timers

### TIM2 - Control PWM de Motores

```
Prescaler (PSC): 32 - 1 = 31
Auto-Reload (ARR): 100 - 1 = 99
Período: (100 × 32) / 32MHz = 100 µs
Frecuencia PWM: 10 kHz

Canal 3 (CH3): Motor Izquierda (PB10)
Canal 4 (CH4): Motor Derecha (PB11)
Duty Cycle: CCR valor (0-99) → 0-99%
```

### TIM3 - Captura de Echo Sensores

```
Prescaler (PSC): 32 - 1 = 31
Auto-Reload (ARR): 0xFFFF
Resolución: 1 µs por tick (32MHz / 32)

Canal 2 (CH2): Echo Delantero (PC7)
  • Modo: Capture Falling/Rising
  • Evento: Cambio de flanco para medir ancho de pulso

Canal 4 (CH4): Echo Trasero (PC9)
  • Modo: Capture Falling/Rising
  • Evento: Cambio de flanco para medir ancho de pulso
```

**Cálculo distancia**:
```
Pulso echo: tiempo en microsegundos
Distancia = (tiempo × velocidad_sonido) / 2
           = (tiempo × 0.034) / 2 cm
```

### TIM4 - Sistema de Timing del Coche

```
Prescaler (PSC): 32 - 1 = 31
Auto-Reload (ARR): 0xFFFF
Resolución: 1 µs por tick

Canal 1 (CH1): Control Triggers Ultrasónicos
  • CCR1 = 10 µs → Trigger ON
  • CCR1 += 50000 → 50 ms espera entre medidas

Canal 2 (CH2): Timing General (cada 1 ms)
  • CCR2 = 1000 µs
  • Incrementa veces_timer
  • Lectura ADC cada 2000 interrupciones = 2 segundos
  • Conteo para giros búsqueda salida

Canal 3 (CH3): PWM Zumbador
  • CCR3 = 50000 µs (50 ms)
  • Genera PWM para buzzer (tono ~1 kHz)
```

---

## 🔊 Sensores Ultrasónicos (HC-SR04)

### Mecanismo de Funcionamiento

```
╔════════════════════════════════════════════════════════════╗
║           HC-SR04 Ultrasonic Distance Sensor              ║
╠════════════════════════════════════════════════════════════╣
║ TRIGGER (entrada)                                          ║
║   • Pulso digital 10 µs → Inicia medición                 ║
║   • Emite onda ultrasónica de 40 kHz                      ║
║                                                            ║
║ ECHO (salida)                                              ║
║   • Pulso proporcional al tiempo de retorno               ║
║   • Ancho = 2 × distancia / velocidad_sonido              ║
║   • Rango: 150 µs a 25 ms (2 cm a 400 cm)                ║
╠════════════════════════════════════════════════════════════╣
║ Velocidad sonido: 340 m/s = 0.034 cm/µs                  ║
║                                                            ║
║ Ejemplo: Objeto a 20 cm                                  ║
║   Tiempo echo = (20 × 2) / 0.034 = 1176 µs              ║
╚════════════════════════════════════════════════════════════╝
```

### Secuencia de Medición (Adelante)

```
1. TIM4_CCR1 interruption (t=0)
   → GPIOC->BSRR |= (1<<6)  // PC6 (Trigger) = 1
   → estado = EMITIENDO
   → Próxima int en +10 µs

2. TIM4_CCR1 interruption (t=10 µs)
   → GPIOC->BSRR |= (1<<22) // PC6 (Trigger) = 0
   → estado = LEYENDO
   → Próxima int en +50 ms

3. PC7 (Echo) sube a 1 → TIM3_CH2 captura (RISING)
   → recibiendo = 1
   → delay_delantero_prev = TIM3->CCR2

4. PC7 (Echo) baja a 0 → TIM3_CH2 captura (FALLING)
   → recibiendo = 0
   → delay_delantero = TIM3->CCR2 - delay_delantero_prev

5. delay_delantero contiene el tiempo en µs → Convertir a distancia
```

### Tabla de Calibración

| Distancia (cm) | Tiempo (µs) | Delay Value |
|----------------|-------------|-------------|
| 5              | 294         | 294         |
| 10             | 588         | 588         |
| 15             | 882         | 882         |
| 20             | 1176        | 1176        |
| 25             | 1470        | 1470        |
| 30             | 1765        | 1765        |

---

## 🎮 Control de Motores

### Configuración PWM

```c
// Estructura de control PWM
Motor_Izquierda:
  ├─ Dirección (PB12): 0 = Adelante, 1 = Atrás
  └─ Velocidad (TIM2_CH3): Valor 1-99 = Duty cycle %

Motor_Derecha:
  ├─ Dirección (PB13): 0 = Adelante, 1 = Atrás
  └─ Velocidad (TIM2_CH4): Valor 1-99 = Duty cycle %
```

### Funciones de Movimiento

```c
avanzar(vel)       // Ambos motores adelante
retroceder(vel)    // Ambos motores atrás
girar_izq(vel)     // Motor derecha adelante, izquierda atrás
girar_der(vel)     // Motor izquierda adelante, derecha atrás
parar()            // Detiene (velocidad = 1)
```

**Nota**: Velocidad = 1 es parada suave, evita daño a transistores.

---

## 📊 Máquina de Estados - Modos de Operación

```
┌──���───────────────────────────────────────────────────────┐
│                   ESTADO INICIAL                         │
│              modo_manual = 1 (Manual)                    │
│            modo = AVANZAR (no aplica)                   │
└────────────────┬─────────────────────────────────────────┘
                 │
     ┌───────────┴────────────────┐
     │                            │
     ▼                            ▼
┌──────────────────┐      ┌───────────────────┐
│   MODO MANUAL    │      │  MODO AUTOMÁTICO  │
│ (modo_manual=1)  │      │ (modo_manual=0)   │
├──────────────────┤      ├───────────────────┤
│ • Recibe cmds    │      │ • Navega solo     │
│   Bluetooth      │      │ • Lee sensores    │
│ • Control remoto │      │ • Ajusta velocidad│
│ • Estado estático│      │ • Busca salida    │
└──────────────────┘      └───────────────────┘
                                │
                    ┌───────────┴────────────┐
                    │                        │
                    ▼                        ▼
            ┌─────────────────┐    ┌──────────────────┐
            │ AVANZAR         │    │ BUSCAR_SALIDA    │
            │ (modo = 0)      │    │ (modo = 1)       │
            ├─────────────────┤    ├──────────────────┤
            │ • Avanza recto  │    │ Pasos 0-4:       │
            │ • Mide distancia│    │ • Giro izq       │
            │ • Si obs → Busca│    │ • Avanza recto   │
            │   salida        │    │ • Giro derecha   │
            └─────────────────┘    │ • Avanza recto   │
                    ▲               │ • Invierte sent  │
                    │               └──────────────────┘
                    │                        │
                    └────────────────────────┘
                    (Salida encontrada)
```

---

## 📈 Máquina de Estados - Búsqueda de Salida

```
┌─ paso_busqueda_salida ─┐
│                         │
├──────────┬──────────┬──────────┬──────────┬────────────┐
│          │          │          │          │            │
▼          ▼          ▼          ▼          ▼            ▼
[0]       [1]       [2]       [3]       [4]         EXIT
 │         │         │         │         │            │
 │    Deshacer   Giro Der  Deshacer  Invierte        │
 │     Giro          │       Giro     Sentido   Regresa a
Giro Izq  │          │         │                 AVANZAR
 │         │         │         │                   │
 ├─────────┤         ├─────────┤                   │
 │ Espera  │         │ Espera  │                   │
 │ 1.1 s   │         │ 1.1 s   │                   │
 │         │         │         │                   │
 ├─ Verif ─┤         ├─ Verif ─┤                   │
 │ dist>20 │◄────────┤ dist>20 │                   │
 │ SI → ex │         │ SI → ex │                   │
 └────┬────┘         └────┬────┘                   │
      │ NO               │ NO                      │
      ├─────────────────┤                          │
      └───────────┬─────────────────────────────────┘
                  │
             Siguiente paso
```

**Tiempos**:
- Espera entre pasos: 1100 ms (tiempo_giro)
- Total secuencia completa: ~4.4 segundos

---

## 🔋 Consumo de Energía (Estimado)

| Componente | Estado | Consumo |
|-----------|--------|---------|
| STM32L152 | Activo | 10 mA |
| Motor x2 | 50% | 400 mA |
| Motor x2 | 100% | 800 mA |
| Zumbador | ON | 50 mA |
| Sensores HC-SR04 | Midiendo | 15 mA |
| Bluetooth HC-05 | Comunicando | 20 mA |
| **Total estimado** | Máximo | **~900 mA** |

---

## 🔐 Mecanismos de Protección

1. **Detección de Obstáculo Cercano**
   - Si distancia < 5 cm → Parada completa
   - Zumbador continuo como alerta

2. **Validación de Comandos**
   - Verificación de rango distancia (5-30 cm)
   - Rechazo de comandos en modo incorrecto

3. **Timeout de Búsqueda**
   - Máximo 4 pasos (4.4 segundos)
   - Si no encuentra salida → Invierte sentido

4. **Limpieza de Estado**
   - Reset automático al cambiar de modo
   - Inicialización de flags e interrupciones

---

## 📋 Variables Globales Críticas

```c
// Modos
char modo_manual;              // 1 = manual, 0 = automático
enum modos_coche modo;         // AVANZAR o BUSCAR_SALIDA

// Sensores
unsigned int delay_delantero;  // Tiempo eco delantero (µs)
unsigned int delay_trasero;    // Tiempo eco trasero (µs)

// Control
char sentido;                  // ADELANTE o ATRAS
unsigned int veces_timer;      // Contador tiempo (1ms tick)
char paso_busqueda_salida;    // 0-4 pasos búsqueda
```

---

## ✅ Checklist de Funcionamiento

- [ ] TIM2: PWM suave en motores, sin saltos
- [ ] TIM3: Captura correcta de ecos (variar distancia)
- [ ] TIM4: Timing consistente (1 ms)
- [ ] ADC: Lectura potenciómetro cada 2 segundos
- [ ] UART: Recepción/transmisión sin corrupción
- [ ] EXTI0: Botón invierte marcha correctamente
- [ ] Modo automático: Detecta obstáculos y busca salida
- [ ] Modo manual: Responde a comandos Bluetooth

---

**Última actualización**: 2025-01-20  
**Autor**: Javier Sánchez
