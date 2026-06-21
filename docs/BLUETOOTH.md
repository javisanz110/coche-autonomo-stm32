# 📱 Protocolo de Comunicación Bluetooth

Documentación completa del protocolo USART/Bluetooth para control remoto del coche.

---

## 🔌 Configuración de Hardware

### USART1 (PA9, PA10)

```
Baudrate: 9600 bps
Data Bits: 8
Stop Bits: 1
Parity: None
Flow Control: None
```

### Módulo Bluetooth HC-05

```
┌─────────────────────────────────┐
│      HC-05 Bluetooth Module     │
├─────────────────────────────────┤
│ VCC → +5V (o 3.3V con resistor) │
│ GND → GND                       │
│ TX → PA10 (RX del STM32)        │
│ RX → PA9 (TX del STM32)         │
│ KEY → NC (No conexión)          │
└─────────────────────────────────┘

PIN por defecto: 1234
Nombre: HC-05
Velocidad: 9600 baud
```

---

## 📤 Formato de Mensajes

### Comando (Móvil → STM32)

```
[COMANDO]\n

Estructura:
├─ Comando (string ASCII)
└─ Terminador (\n = 0x0A = salto de línea)

Longitud máxima: 10 caracteres
```

### Respuesta (STM32 → Móvil)

```
[RESPUESTA]\n

Estructura:
├─ Mensaje de confirmación
└─ Terminador (\n)

La aplicación debe capturar hasta \n
```

---

## 🎮 Comandos Soportados

### Modo Manual - Movimiento

| Comando | Descripción | Respuesta |
|---------|-------------|-----------|
| `GO` | Avanzar hacia adelante | `hacia delante` |
| `BACK` | Retroceder hacia atrás | `hacia atras` |
| `IZQ` | Girar izquierda (contramarcha) | `hacia la izquierda` |
| `DER` | Girar derecha (contramarcha) | `hacia la derecha` |
| `STP` | Detener movimiento | `parar` |

### Modo Manual - Configuración

| Comando | Descripción | Validación | Respuesta |
|---------|-------------|-----------|-----------|
| `MIN[xx]` | Establecer distancia mínima (cm) | 5-30 cm | `distancia min actualizada` |
| `MAX[xx]` | Establecer distancia máxima (cm) | 5-30 cm | `distancia max actualizada` |

**Ejemplos**:
```
MIN5    → Distancia mínima = 5 cm
MAX30   → Distancia máxima = 30 cm
MIN3    → ❌ Error: < 5 cm
MAX35   → ❌ Error: > 30 cm
MAX10   → ❌ Error: MIN > MAX
```

### Modo de Operación

| Comando | Descripción | Respuesta |
|---------|-------------|-----------|
| `MODE_MANUAL` | Cambiar a control remoto | `modo manual: ON` |
| `MODE_AUTO` | Cambiar a navegación autónoma | `modo automático establecido` |

---

## 🔄 Flujo de Comunicación

### Inicialización del Sistema

```
┌─────────────────────────────────────────┐
│  STM32 inicia (main.c)                  │
├─────────────────────────────────────────┤
│ 1. USART1 configurado (9600 baud)       │
│ 2. HAL_UART_Transmit() → "ready!\n"     │
│ 3. HAL_UART_Receive_IT() habilitado     │
│ 4. Esperando comandos en interrupción   │
└─────────────────────────────────────────┘
        │
        │ Mensaje: "ready!\n"
        ▼
┌─────────────────────────────┐
│  Aplicación Móvil           │
├─────────────────────────────┤
│ • Lee "ready!"              │
│ • Muestra "Conectado ✓"     │
│ • Habilitado para enviar cmds│
└─────────────────────────────┘
```

### Recepción de Comando

```
┌────────────────────────────────────────────────────────┐
│ Aplicación Móvil envía: "GO\n" (3 bytes)              │
├────────────────────────────────────────────────────────┤
│ Byte 0: 'G' (0x47)                                    │
│ Byte 1: 'O' (0x4F)                                    │
│ Byte 2: '\n' (0x0A)                                   │
└────────────────────────────────────────────────────────┘
        │
        │ USART RX pin (PA10)
        ▼
┌────────────────────────────────────────────────────────┐
│ STM32 - HAL_UART_RxCpltCallback()                     │
├────────────────────────────────────────────────────────┤
│ Recibido 'G':                                          │
│   orden[0] = 'G'                                       │
│   num_letra = 1                                        │
│   Re-habilitar RX interrupción                         │
│                                                        │
│ Recibido 'O':                                          │
│   orden[1] = 'O'                                       │
│   num_letra = 2                                        │
│   Re-habilitar RX interrupción                         │
│                                                        │
│ Recibido '\n':                                         │
│   orden[2] = '\0' ← Fin de string                      │
│   ✓ ejecutar_orden("GO")                              │
│   → Devuelve: mode = ahead                            │
│   → send_msg(ahead)                                   │
│   → Transmit: "hacia delante\n"                       │
│   Limpiar orden[] y num_letra                         │
│   Re-habilitar RX interrupción                         │
└────────────────────────────────────────────────────────┘
        │
        │ USART TX pin (PA9)
        ▼
┌────────────────────────────────────────────────────────┐
│ Aplicación Móvil recibe: "hacia delante\n"            │
├────────────────────────────────────────────────────────┤
│ • Parsea respuesta                                     │
│ • Muestra en pantalla: "ADELANTE"                     │
│ • Coche inicia movimiento                             │
└────────────────────────────────────────────────────────┘
```

---

## 💻 Implementación en STM32

### Configuración USART

```c
// En main.c - Configuración de pines PA9/PA10
GPIOA->MODER |= (1<<(9*2+1));   // PA9 AF
GPIOA->MODER &= ~(1<<(9*2));

GPIOA->MODER |= (1<<(10*2+1));  // PA10 AF
GPIOA->MODER &= ~(1<<(10*2));

// Función alternativa USART1
GPIOA->AFR[1] |= 0x00000770;    // AF7 para USART1

// USART1 - 9600 baud, 8N1
USART1->CR1 = 0x0000;
USART1->BRR = 0x00D6;           // Para 32 MHz
USART1->CR1 |= (1<<2);          // RX enable
USART1->CR1 |= (1<<3);          // TX enable
USART1->CR1 |= (1<<5);          // RXNE interrupt
USART1->CR1 |= (1<<0);          // USART enable

// NVIC para interrupción USART1
NVIC->ISER[1] |= (1<<5);        // USART1_IRQn = 37
```

### Callback de Recepción

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    if(letra == '\n'){
        // Fin de comando
        orden[num_letra-1] = '\0';      // Terminar string
        mode = ejecutar_orden(orden);   // Procesar
        send_msg(mode);                 // Responder
        
        // Limpiar buffer
        for(int i = 0; i<10; i++)
            orden[i] = '\0';
        num_letra = 0;
    }
    else{
        // Acumular carácter
        orden[num_letra] = letra;
        num_letra++;
    }
    
    // Re-habilitar recepción
    HAL_UART_Receive_IT(&huart1, &letra, 1);
}
```

### Envío de Mensaje

```c
void send_msg(enum msg_index i){
    HAL_UART_Transmit(&huart1, 
                      (unsigned char*)msgs[i], 
                      strlen(msgs[i]), 
                      tiempo_limite);
}
```

---

## 📱 Aplicación Android Recomendada

### Requisitos Mínimos
- Android 5.0+
- Bluetooth 2.1+

### Interfaz Sugerida

```
┌─────────────────────────────────┐
│   🚗 Control Coche Autónomo    │
├─────────────────────────────────┤
│                                 │
│        [ ↑ ]                    │
│    [ ← ] [ STOP ] [ → ]         │
│        [ ↓ ]                    │
│                                 │
│  Estado: [Conectado ✓]          │
│  Velocidad: [████░░]  50%       │
│                                 │
│  Modo:                          │
│  [ Auto ] [ Manual ]            │
│                                 │
│  Configuración:                 │
│  Min dist: [_5_] cm             │
│  Max dist: [30_] cm             │
│                                 │
│  [Conectar] [Desconectar]       │
└─────────────────────────────────┘
```

### Comportamiento de Botones

| Botón | Presionado | Soltado |
|-------|-----------|---------|
| ↑ (Adelante) | Envía `GO` | Envía `STP` |
| ↓ (Atrás) | Envía `BACK` | Envía `STP` |
| ← (Izq) | Envía `IZQ` | Envía `STP` |
| → (Der) | Envía `DER` | Envía `STP` |
| Min | Envía `MIN[xx]` | - |
| Max | Envía `MAX[xx]` | - |

---

## ⚠️ Casos de Error

### Error: Comando No Válido

```
Enviado: "XYZ\n"
Respuesta: "comando no válido"

Causas posibles:
- Typo en comando
- Comando no soportado
- Argumentos incorrectos
```

### Error: Modo Manual No Establecido

```
Enviado: "GO\n" (en modo automático)
Respuesta: "modo manual no establecido :("

Solución:
- Enviar: "MODE_MANUAL\n" primero
- Esperar: "modo manual: ON"
- Luego enviar: "GO\n"
```

### Error: Comando Duplicado

```
Enviado: "MODE_MANUAL\n"
Respuesta: "modo manual establecido"

Enviado: "MODE_MANUAL\n" (nuevamente)
Respuesta: "modo manual establecido"
```

**Nota**: No hay error, el sistema acepta el comando redundante.

---

## 🧪 Prueba de Comunicación

### Usando Terminal Serial (Linux/Mac)

```bash
# Conectar a puerto serial
screen /dev/ttyUSB0 9600

# Ver respuesta del coche
> ready!

# Enviar comando
> GO
< hacia delante

# Parar
> STP
< parar
```

### Usando PuTTY (Windows)

1. Seleccionar puerto COM (HC-05 emparejado)
2. Configurar: 9600, 8, N, 1
3. Conectar
4. Ver "ready!"
5. Tipear comandos + Enter

---

## 📊 Diagrama Temporal de Comunicación

```
Tiempo    Evento
─────────────────────────────────────────────
0 ms      Aplicación envía 'G' (0x47)
          USART1_RxCpltCallback ejecuta
          orden[0] = 'G'
          num_letra = 1

5 ms      Aplicación envía 'O' (0x4F)
          USART1_RxCpltCallback ejecuta
          orden[1] = 'O'
          num_letra = 2

10 ms     Aplicación envía '\n' (0x0A)
          USART1_RxCpltCallback ejecuta
          orden[2] = '\0'
          ejecutar_orden("GO")
          
15 ms     mode = ahead
          send_msg(ahead)
          HAL_UART_Transmit comienza
          
20 ms     "h" (0x68) enviado por TX (PA9)
          
25 ms     "a" enviado
...
50 ms     "\n" (0x0A) enviado
          Transmisión completa
```

---

## 🔐 Seguridad y Validación

### Validación de Entrada

```c
if(strncmp(orden, "MIN", 3) == 0){
    char num[4];
    strcpy(num, orden+3);           // Extraer "15" de "MIN15"
    dist_min = strtol(num, NULL, 10);
    
    // Validar rango
    if(dist_min < 5 || dist_min > dist_max)
        return invalid;              // ❌ Fuera de rango
    
    return min_update;               // ✓ Válido
}
```

### Buffer Overflow Prevention

```c
#define MAX_ORDEN_LEN 10

if(num_letra >= MAX_ORDEN_LEN){
    // Evitar desbordamiento
    num_letra = MAX_ORDEN_LEN - 1;
    orden[num_letra] = '\0';
}
```

---

## 📈 Rendimiento

| Métrica | Valor |
|---------|-------|
| Baudrate | 9600 bps = 960 bytes/s |
| Latencia comando | ~50-100 ms |
| Máximo buffer | 10 caracteres |
| Timeout transmisión | 10000 ms (configurable) |
| Frecuencia lectura ADC | Cada 2 segundos |

---

## ✅ Checklist de Pruebas

- [ ] Coche envía "ready!" al iniciar
- [ ] Recibe comando "GO" sin corrupción
- [ ] Responde "hacia delante"
- [ ] Responde "comando no válido" a entrada incorrecta
- [ ] Valida rangos MIN/MAX correctamente
- [ ] Cambia de modo con MODE_MANUAL/MODE_AUTO
- [ ] Sin delay entre comando y respuesta
- [ ] Buffer se limpia después de cada comando

---

**Última actualización**: 2025-01-20  
**Autor**: Javier Sánchez
