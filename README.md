# Full-Colombia-Wayne
Communication program with fuel pumps Wayne.

## Recibe los siguientes comandos desde el uC principal.

### La funcion AUTORIZAR
Llega el comando con sus respectivos datos de autorizacion:

typedef struct
{
  byte lado;
  int mang;
  byte modo;
  double cantidad;
} I2cAutoriza;

Ademas de cargar la informacion de autorizacion, este comando no ejecuta nada mas, y tiene solo efecto cuando se levanta una manguera.
Dentro de la funcion: int		autorizar(byte ID, byte manguera, byte modo, long cantidad, byte *precioBDC)
solo maneja un proceso de autorizacion al tiempo, lo que quiere decir que si llega una nueva autorizacion a la otra cara, esta reeemplaza la autorizacion anterior.

