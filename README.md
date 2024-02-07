# Full-Colombia-Wayne
Communication program with fuel pumps Wayne.

## Recibe los siguientes comandos desde el uC principal.

### La funcion AUTORIZAR
Llega el comando con sus respectivos datos de autorizacion:

typedef struct
{
  uint8_t lado;
  int mang;
  uint8_t modo;
  double cantidad;
} I2cAutoriza;

Ademas de cargar la informacion de autorizacion, este comando no ejecuta nada mas, y tiene solo efecto cuando se levanta una manguera.
Dentro de la funcion: int		autorizar(uint8_t ID, uint8_t manguera, uint8_t modo, long cantidad, uint8_t *precioBDC)
solo maneja un proceso de autorizacion al tiempo, lo que quiere decir que si llega una nueva autorizacion a la otra cara, esta reeemplaza la autorizacion anterior.

