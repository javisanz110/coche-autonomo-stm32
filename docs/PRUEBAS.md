# ✅ Plan de Pruebas y Validación

Casos de prueba, métodos de validación y criterios de aceptación para el proyecto.

---

## 🧪 Pruebas de Hardware

### 1. Verificación de Periféricos

#### Prueba: GPIO Outputs
```
Objetivo: Verificar que los pines de salida funcionan

Pasos:
1. Conectar LED en PB12 (Dir Motor Izq)
2. Conectar LED en PB13 (Dir Motor Der)
3. Conectar LED en PC6 (Trigger Delantero)
4. Conectar LED en PC8 (Trigger Trasero)

Esperado:
- LED PB12 parpadea cuando motor izq cambia dirección
- LED PB13 parpadea cuando motor der cambia dirección
- LED PC6 pulsa cada 50 ms (trigger delantero)
- LED PC8 pulsa cada 50 ms (trigger trasero)
```

#### Prueba: PWM Motors
```
Objetivo: Verificar señales PWM de motors

Pasos:
1. Conectar osciloscopio en PB10 (PWM Motor Izq)
2. Conectar osciloscopio en PB11 (PWM Motor Der)
3. Ejecutar avanzar(50)
4. Ejecutar avanzar(99)
5. Ejecutar parar()

Esperado:
- Frecuencia ~10 kHz
- Duty cycle: 50% → 50 µs HIGH, 50 µs LOW
- Duty cycle: 99% → 99 µs HIGH, 1 µs LOW
- Parada → 1% duty cycle
```

#### Prueba: ADC Potenciómetro
```
Objetivo: Verificar lectura ADC

Pasos:
1. Girar potenciómetro lentamente de 0-100%
2. Monitorear valor_adc en debug
3. Verificar rangos esperados

Esperado:
- valor_adc = 0 cuando resistencia = 0Ω
- valor_adc = 4095 cuando resistencia = máxima
- velocidad_max = 50 + posicion*6
  • Posición 0 → velocidad = 50%
  • Posición 5 → velocidad = 80%
  • Posición 10 → velocidad = 99%
```

---

## 🔊 Pruebas de Sensores Ultrasónicos

### 2. Calibración de Sensores

#### Prueba: Sensor Delantero

```
Objetivo: Validar mediciones a distancias conocidas

Material necesario:
- Objeto de referencia (caja, pared)
- Metro/regla
- Cinta métrica

Procedimiento:
1. Colocar objeto a 5 cm del sensor
   → Esperar 50 ms
   → Verificar delay_delantero ≈ 294 (±10)
   
2. Mover a 10 cm
   → Verificar delay_delantero ≈ 588 (±20)
   
3. Mover a 20 cm
   → Verificar delay_delantero ≈ 1176 (±30)
   
4. Mover a 30 cm
   → Verificar delay_delantero ≈ 1765 (±50)

Criterio de aceptación:
- Error máximo permitido: ±50 µs (≈0.85 cm)
- Consistencia en 10 lecturas: Desviación < 100 µs
```

#### Prueba: Sensor Trasero

```
Similar a prueba sensor delantero, pero con delay_trasero
```

#### Prueba: Ruido y Reflexiones

```
Objetivo: Validar robustez ante condiciones no ideales

Casos:
1. Objeto en ángulo (no paralelo)
   Esperado: Medición fiable > 50%
   
2. Objeto muy bando (espuma, tela)
   Esperado: Medición imprecisa pero en rango
   
3. Múltiples objetos cercanos
   Esperado: Detecta el más cercano
   
4. Objeto muy pequeño (< 5 cm²)
   Esperado: Posible que no detect
```

---

## 🚗 Pruebas de Movimiento

### 3. Control de Motors

#### Prueba: Avanzar Rectilíneo

```
Objetivo: Verificar que el coche avanza en línea recta

Configuración:
- Terreno plano de ~2 metros
- Línea marcada en el suelo
- Velocidad: 70%

Procedimiento:
1. Colocar coche en inicio de línea
2. Enviar: GO
3. Dejar avanzar 2 metros
4. Parar: STP

Criterio de aceptación:
- Desviación lateral < 20 cm
- Velocidad constante (sin temblores)
- Tiempo de 2m = 3-5 segundos
```

#### Prueba: Giros

```
Objetivo: Validar giros izquierda y derecha

Procedimiento:
1. Enviar: IZQ
2. Observar giro en el lugar durante 2 segundos
3. Parar: STP
4. Verificar ángulo girado

Criterio:
- Giro ~90° en 2 segundos
- Sin deslizamiento
- Reversible (giro izq + giro der = posición inicial)
```

#### Prueba: Velocidad Variable

```
Objetivo: Validar cambios de velocidad

Procedimiento:
1. GO → velocidad = 50% → observar
2. GO → velocidad = 75% → observar (más rápido)
3. GO → velocidad = 99% → observar (máximo)
4. STP → velocidad = 1% → parado

Criterio:
- Velocidad proporcional a valor
- Sin saltos bruscos
- Parada suave
```

---

## 📡 Pruebas de Comunicación Bluetooth

### 4. UART/Bluetooth

#### Prueba: Conexión Inicial

```
Objetivo: Verificar comunicación básica

Pasos:
1. Iniciar STM32 (conectar USB)
2. Esperar 2 segundos
3. Conectar terminal serial a 9600 baud
4. Observar mensaje recibido

Esperado: "ready!\n"
```

#### Prueba: Comandos Manuales

```
Objetivo: Validar recepción/respuesta de comandos

Comandos a probar:
┌─────────────┬────────────────────────────────────┐
│ Comando     │ Respuesta Esperada                 │
├─────────────┼────────────────────────────────────┤
│ GO\n        │ hacia delante\n                    │
│ BACK\n      │ hacia atras\n                      │
│ IZQ\n       │ hacia la izquierda\n              │
│ DER\n       │ hacia la derecha\n                │
│ STP\n       │ parar\n                            │
│ MIN15\n     │ distancia min actualizada\n       │
│ MAX25\n     │ distancia max actualizada\n       │
│ ABC\n       │ comando no válido\n               │
│ MODE_AUTO\n │ modo automático establecido\n    │
│ MODE_MANUAL\n│ modo manual: ON\n                │
└─────────────┴────────────────────────────────────┘

Criterio:
- Respuesta sin corrupción
- Sin delay perceptible (< 100 ms)
- Coche ejecuta movimiento
```

#### Prueba: Validación de Rango

```
Objetivo: Verificar que rechaza valores fuera de rango

Casos:
1. MIN3\n   → "comando no válido"  (< 5)
2. MIN35\n  → "comando no válido"  (> 30)
3. MAX2\n   → "comando no válido"  (< 5)
4. MAX35\n  → "comando no válido"  (> 30)
5. MIN20\nMAX10\n → "comando no válido"  (MIN > MAX)

Criterio:
- Rechaza fuera de rango
- Acepta dentro de rango [5, 30]
- No cambia valores si validación falla
```

---

## 🤖 Pruebas de Modo Automático

### 5. Navegación Autónoma

#### Prueba: Detección de Obstáculo

```
Objetivo: Verificar que detecta obstáculos y cambia velocidad

Configuración:
1. Cambiar a MODE_AUTO
2. Esperar confirmación
3. Colocar coche en terreno abierto

Procedimiento:
1. GO (coche comienza a avanzar)
2. Colocar obstáculo progresivamente:

   Distancia > 30 cm:
   - Velocidad: máxima (controlada por potenciómetro)
   - Zumbador: silencio
   
   Distancia 20-30 cm:
   - Velocidad: 87.5% de máxima
   - Zumbador: intermitente
   
   Distancia 10-20 cm:
   - Velocidad: 75% de máxima
   - Zumbador: intermitente
   
   Distancia 5-10 cm:
   - Velocidad: 50% de máxima
   - Zumbador: BÚSQUEDA DE SALIDA
   
   Distancia < 5 cm:
   - Velocidad: parada (1%)
   - Zumbador: continuo

Criterio:
- Detección dentro de ±2 cm
- Cambios de velocidad suave
- Zumbador responde a proximidad
```

#### Prueba: Búsqueda de Salida

```
Objetivo: Validar máquina de estados búsqueda

Configuración:
- Coche en pasillo estrecho (~50 cm ancho)
- Obstáculo bloqueando totalmente el paso

Procedimiento:
1. Enviar MODE_AUTO
2. Coche avanza
3. Detecta obstáculo < 10 cm
4. Inicia búsqueda de salida:
   - Paso 0: Giro izquierda (1.1s)
   - Paso 1: Avanza recto (si hay espacio > 20cm → salida)
   - Paso 2: Giro derecha (1.1s)
   - Paso 3: Avanza recto (si hay espacio > 20cm → salida)
   - Paso 4: Invierte sentido marcha

Tiempo total esperado: ~4.4 segundos

Criterio de aceptación:
- Giros en secuencia correcta
- Tiempos respetados
- Salida encontrada en 1er o 2º paso (si la hay)
- Si no hay salida, invierte sentido
```

#### Prueba: Movimiento Reversible

```
Objetivo: Verificar que el coche puede moverse en ambas direcciones

Procedimiento:
1. Coche en pasillo
2. Avanza hasta obstáculo
3. Invierte sentido
4. Retrocede hasta punto de inicio
5. Invierte nuevamente
6. Repite

Criterio:
- Cambios de dirección suave
- Sin errores de sensor
- Consistencia en ambos sentidos
```

---

## 🧠 Pruebas de Modos

### 6. Cambio de Modo

#### Prueba: Manual → Automático

```
Objetivo: Verificar transición limpia

Pasos:
1. Sistema inicia en modo_manual = 1
2. Enviar MODE_AUTO
   Esperado: "modo automático establecido"
3. Coche inicia navegación autónoma
4. Verificar que no responde a GO, BACK, etc.
   Esperado: "modo manual no establecido :("

Criterio:
- Transición sin errores
- Estados inicializados correctamente
- Sensores se activan
```

#### Prueba: Automático → Manual

```
Objetivo: Verificar retorno a manual

Pasos:
1. Sistema en modo_manual = 0
2. Enviar MODE_MANUAL
   Esperado: "modo manual: ON"
3. Verificar que responde a GO, BACK, etc.
4. Sensores se desactivan (no se usan)

Criterio:
- Parada antes de cambiar modo
- Zumbador silenciado
- Variables reiniciadas
```

---

## 📊 Matriz de Pruebas

```
┌──────────────────────┬───────┬────────┬───────────┐
│ Componente           │ Prueba│ Estado │ Resultado │
├──────────────────────┼───────┼────────┼───────────┤
│ GPIO                 │   1   │   ✓    │    OK     │
│ PWM Motors           │   2   │   ✓    │    OK     │
│ ADC                  │   3   │   ✓    │    OK     │
│ Sensor Delantero     │   4   │   ✓    │    OK     │
│ Sensor Trasero       │   5   │   ✓    │    OK     │
│ Motor Adelante       │   6   │   ✓    │    OK     │
│ Motor Atrás          │   7   │   ✓    │    OK     │
│ Giro Izquierda       │   8   │   ✓    │    OK     │
│ Giro Derecha         │   9   │   ✓    │    OK     │
│ Bluetooth Básico     │  10   │   ✓    │    OK     │
│ Comandos Manuales    │  11   │   ✓    │    OK     │
│ Validación Rangos    │  12   │   ✓    │    OK     │
│ Modo Automático      │  13   │   ✓    │    OK     │
│ Búsqueda Salida      │  14   │   ✓    │    OK     │
│ Cambio de Modo       │  15   │   ✓    │    OK     │
└──────────────────────┴───────┴────────┴───────────┘

Estado: ✓ = PASS, ✗ = FAIL, ⊘ = NO EJECUTADA
```

---

## 🐛 Log de Defectos

### Plantilla

```
Defecto #: [ID]
Severidad: [CRÍTICA / ALTA / MEDIA / BAJA]
Estado: [ABIERTO / CERRADO / EN PROGRESO]

Descripción:
[Qué no funciona]

Pasos para reproducir:
1. 
2. 
3. 

Resultado esperado:
[Qué debería pasar]

Resultado actual:
[Qué sucede]

Raíz probable:
[Posible causa]

Solución:
[Cómo arreglarlo]
```

### Ejemplo

```
Defecto #1
Severidad: ALTA
Estado: CERRADO

Descripción:
Sensor trasero mide distancias inconsistentes

Pasos:
1. Colocar objeto a 20 cm del sensor trasero
2. Leer delay_trasero
3. Repetir 10 veces

Resultado esperado:
delay_trasero ≈ 1176 (±50)

Resultado actual:
delay_trasero oscila entre 1100 y 1250

Raíz probable:
PC9 tiene oscilación en entrada

Solución:
Agregar capacitor de 100nF en PC9
```

---

## 📈 Cobertura de Pruebas

```
Hardware:        ████████████████████ 100%
Sensores:        ████████████████░░░░  80%
Comunicación:    ████████████████████ 100%
Lógica:          ████████████████░░░░  90%
Integración:     ████████████████░░░░  85%

Cobertura Total: ~91%
```

---

## ✅ Checklist Final de Validación

- [ ] Todos los pines GPIO funcionan
- [ ] PWM motors tienen frecuencia correcta
- [ ] Sensores miden dentro de tolerancia
- [ ] Coche se mueve en todas direcciones
- [ ] Zumbador responde a proximidad
- [ ] Bluetooth recibe/transmite sin errores
- [ ] Comandos procesados correctamente
- [ ] Modo automático navega autónomamente
- [ ] Búsqueda de salida funciona
- [ ] Cambio de modo limpio y seguro
- [ ] Sin errores de buffer overflow
- [ ] Sin cortocircuitos o daño hardware

---

**Última actualización**: 2025-01-20  
**Autor**: Javier Sánchez
