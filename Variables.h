/* ***************************************************************************************************
 *                                               VENTAS                                              *
 *****************************************************************************************************/
typedef struct                      // Dato que se acaba de leer desde el protocolo.
{
	double			Volumen;        // 4
	unsigned long	Venta;          // 4
	unsigned int	PPU;            // 2
	
	unsigned long	Numeracion;     // 4
}Venta;                             // 14
Venta   venta[12];                   // 84

/* ***************************************************************************************************
 *                                          COMUNICACION i2c                                         *
 *****************************************************************************************************/
typedef struct
{
	byte funcion;
	unsigned long time;
} I2cFuncion;
I2cFuncion	i2cFuncion;

char  DatosI2C[20];

// ------------------------------------------------------------------------
typedef struct    // 3 bytes
{
	byte manguera;
	unsigned int PPU;
} I2CPrecio;

// ------------------------------------------------------------------------
typedef struct
{
	byte lado;
	int mang;
	byte modo;
	double cantidad;
} I2cAutoriza;
I2cAutoriza   i2cAutoriza;

/* ***************************************************************************************************
 *                                               iButton                                             *
 *****************************************************************************************************/
OneWire   ds1(2);			// Pin para iButton1
OneWire   ds2(3);			// Pin para iButton2

String    keyStatus="";
byte      uso_iButton=0;
unsigned long iB_Tini;

typedef struct
{
	char  id[18];
	byte  lado;
	int   Check;
}Ibutton;

/* ***************************************************************************************************
 *																									 *
 *****************************************************************************************************/
typedef struct
{
	byte	Num_Caras = 2;
	byte	Num_Mang_1 = 2;
	byte	Num_Mang_2 = 0;
}	Configuracion;
Configuracion Conf;
