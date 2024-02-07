/* ***************************************************************************************************
 *                                               VENTAS                                              *
 *****************************************************************************************************/
typedef struct                      					// Dato que se acaba de leer desde el protocolo. 15Bytes.
{
	double			Volumen;
	unsigned long	Venta;
	unsigned int	PPU;
	uint8_t	manguera;

	unsigned long	Numeracion;
}Venta;
Venta   venta[3][2];                   					// 3 surtidores, 2 lados.

unsigned long	numeracion[3][2][4];					// 3 surtidores, 2 lados, 4 mangueras. 96 bytes.

/* ***************************************************************************************************
 *                                          COMUNICACION i2c                                         *
 *****************************************************************************************************/
typedef struct
{
	uint8_t funcion;
	unsigned long time;
} I2cFuncion;
I2cFuncion	i2cFuncion;

char  DatosI2C[20];

int		bytesWrite=0;
char	txData[190];

// ------------------------------------------------------------------------
typedef struct    // 3 uint8_ts
{
	uint8_t manguera;
	unsigned int PPU;
} I2CPrecio;

// ------------------------------------------------------------------------
typedef struct
{
	uint8_t surtidor;
	uint8_t lado;
	int mang;
	uint8_t modo;
	double cantidad;
} I2cAutoriza;
I2cAutoriza   i2cAutoriza;

/* ***************************************************************************************************
 *                                               iButton                                             *
 *****************************************************************************************************/
OneWire   ds1(2);			// Pin para iButton1
OneWire   ds2(3);			// Pin para iButton2

String    keyStatus="";
uint8_t      uso_iButton=0;
unsigned long iB_Tini;

typedef struct
{
	char  id[18];
	uint8_t  lado;
	int   Check;
}Ibutton;

/* ***************************************************************************************************
 *                                               CONFIG                                              *
 *****************************************************************************************************/
typedef struct
{
	uint8_t	Num_Surt = 1;
	uint8_t	Num_Mang[3] = { 1, 0, 0 };
}	Configuracion;
Configuracion Conf;
