# 🚗 Coche Autónomo con Detección de Obstáculos - STM32L152C

> **Proyecto Académico**: Sistemas Digitales Basados en Microprocesadores  
> Diseño e implementación de un vehículo autónomo controlado por STM32 programado en C a nivel de registro

---

## 📋 Descripción del Proyecto

Sistema completo de un vehículo autónomo controlado por **microcontrolador STM32L152C** que integra:

- **Detección de obstáculos**: Dos sensores ultrasónicos (HC-SR04) adelante y atrás
- **Control de motores**: Señales PWM para velocidad y dirección
- **Control de velocidad**: Potenciómetro analógico vía ADC para ajustar velocidad máxima (0-99%)
- **Sistema de alarma**: Zumbador con avisos de proximidad (continuo e intermitente)
- **Control remoto**: Comunicación Bluetooth vía USART para control desde móvil
- **Inteligencia**: Máquina de estados para buscar salida ante obstáculos

---

## 🔧 Características Técnicas

### Microcontrolador
- **Placa**: STM32L152C Discovery
- **IDE**: STM32CubeIDE
- **Lenguaje**: C puro - **programación a nivel de registro (sin librerías de alto nivel)**

### Periféricos Utilizados

| Periférico | Función | Configuración |
|-----------|---------|---------------|
| **TIM2** | Control PWM motores | Canales 3-4, Prescaler 32 |
| **TIM3** | Captura echo sensores ultrasónicos | Canales 2 y 4 (PC7, PC9) |
| **TIM4** | Timing de triggers y buzzer | Canales 1-3, interrupciones cada 1ms |
| **ADC1** | Lectura potenciómetro | Canal 4 (PA2), 12 bits, cada 2s |
| **USART1** | Comunicación Bluetooth | 9600 baud, RX/TX |
| **EXTI0** | Botón cambio sentido marcha | Interrupción rising PA0 |

---

## 📚 Documentación

Ver archivos en `/docs`:
- **ARQUITECTURA.md** - Diagrama de periféricos y configuración de timers
- **SENSORES.md** - Guía completa de sensores ultrasónicos HC-SR04
- **BLUETOOTH.md** - Protocolo de comunicación UART/Bluetooth
- **PRUEBAS.md** - Plan de validación y casos de prueba

---

## 🚀 Quick Start

```bash
git clone https://github.com/javisanz110/coche-autonomo-stm32.git
cd coche-autonomo-stm32
```

1. Abrir en STM32CubeIDE
2. Compilar (Ctrl+B)
3. Flashear en STM32L152C
4. Ver main.c para código completo

---

## 👨‍💻 Autor

**Javier Sánchez** (@javisanz110)

---

**⭐ Si te resulta útil, deja una estrella**
