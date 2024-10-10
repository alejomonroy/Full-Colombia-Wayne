/* ***************************************************************************************************
 *                                               VENTAS                                              *
 *****************************************************************************************************/
unsigned long SYNC=1;

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
uint8_t			mang_status[3][2] = { {0,0}, {0 ,0}, {0 ,0} };				// por CARA.

/* ***************************************************************************************************
 *                                          COMUNICACION i2c                                         *
 *****************************************************************************************************/
typedef struct
{
	uint8_t funcion =0;
	unsigned long time =0;

	int mang;
	uint8_t modo;
	double cantidad;
} I2cFuncion;
I2cFuncion  i2cFuncion;

I2cFuncion  funAuth[3][2];    // 3 surtidores, 2 caras.

char  DatosI2C[20];

int		bytesWrite=0;
char	txData[200];

// ------------------------------------------------------------------------
typedef struct    // 3 uint8_ts
{
	uint8_t surtidor;
	uint8_t lado;
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
	uint8_t	Num_Surt = 2;                 // Numero de surtidores conectador a este promini.
	uint8_t	Num_Mang[3] = { 2, 1, 0 };    // Numero de mangueras por lado de cada surtidor.
}	Configuracion;
Configuracion Conf;
