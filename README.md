# ProMini Protocolo de Comunicación con Surtidores

Este proyecto implementa un protocolo de comunicación entre un Arduino Pro Mini y surtidores de combustible utilizando RS485 y SoftwareSerial. Además, se integran funcionalidades para la gestión de ventas, precios y configuraciones i2c.

## Archivos del Proyecto

### 1. **ProMini_Protocolo.ino**
Archivo principal que contiene la lógica del loop y funciones que orquestan las interacciones entre el hardware y las funcionalidades descritas.

### 2. **ProMini_Protocolo.h**
Define funciones clave y constantes relacionadas con el protocolo de comunicación.

### 3. **Protocolo_Wayne.h**
Implementa las funciones específicas del protocolo de comunicación Wayne, que incluyen:
- Gestión de precios.
- Consulta de estado.
- Autorización y desautorización de ventas.

### 4. **Variables.h**
Declara las estructuras y variables globales, como:
- Información de ventas (volumen, precio, numeración).
- Configuraciones del sistema.
- Estructuras para comunicación i2c.

## Estructuras Principales

### 1. **Venta**
Estructura que almacena la información de las ventas, incluyendo:
- Volumen.
- Venta total.
- Precio por unidad (PPU).
- Numeración y manguera asociada.

### 2. **I2cFuncion e I2cAutoriza**
Estructuras para gestionar funciones y autorizaciones a través del protocolo i2c.

### 3. **Configuracion**
Permite definir el número de surtidores y mangueras conectadas al sistema.

## Funcionalidades Principales

### 1. **Comunicación RS485**
El proyecto utiliza RS485 para enviar y recibir tramas de datos con los surtidores. Las funciones implementadas incluyen:
- `EnviarID(uint8_t ID)`: Envía el ID del dispositivo conectado.
- `RecibirTrama(unsigned char *trama)`: Recibe tramas y verifica su integridad.
- `CerrarComunicacion(uint8_t ID, uint8_t consecutivo)`: Finaliza la comunicación con el dispositivo.

### 2. **Protocolo Wayne**
El protocolo implementa las siguientes operaciones:
- **Estado:** Consulta el estado de los surtidores y mangueras.
- **Totales:** Recupera los totales de ventas por manguera.
- **Autorización:** Autoriza transacciones basadas en configuraciones predefinidas.
- **Precios:** Actualiza precios en los surtidores conectados.

### 3. **Gestión de iButton**
El proyecto incluye soporte para dispositivos iButton conectados a través de OneWire, utilizados para identificación y control de acceso.

### 4. **EEPROM**
Se utiliza para almacenar configuraciones y datos persistentes, con funciones para lectura y escritura en memoria EEPROM.

## Dependencias
- Arduino IDE (compatible con Pro Mini).
- Librerías:
  - `EEPROM.h`.
  - `OneWire.h`.
  - `SoftwareSerial.h`.

## Configuración
1. Conectar los dispositivos RS485 al Pro Mini utilizando los pines definidos en el código (TXE485 y RXE485).
2. Configurar las estructuras de `Configuracion` y `I2cFuncion` según el número de surtidores y mangueras.
3. Cargar el sketch desde el archivo `.ino` en el Pro Mini mediante Arduino IDE.

## Uso
- Para inicializar la comunicación, llamar a las funciones de configuración y luego ejecutar el `loop` principal.
- Para pruebas, se pueden monitorear las salidas en el monitor serial del Arduino IDE.

## Consideraciones
- Verificar la integridad de las conexiones RS485.
- Asegurar que las configuraciones de los surtidores coincidan con los IDs y parámetros definidos en el sistema.
- Monitorear el rendimiento y realizar ajustes en los tiempos de delay según las especificaciones del hardware.

## Contacto
Para soporte o consultas, por favor contactar a [Luis Alejandro Monroy](https://github.com/alejomonroy).

