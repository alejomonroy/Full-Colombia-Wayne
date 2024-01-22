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
Venta   venta[6];                   // 84

/* ***************************************************************************************************
 *                                          COMUNICACION i2c                                         *
 *****************************************************************************************************/
typedef struct
{
	byte funcion;
	unsigned long time;
} I2cFuncion;
I2cFuncion	i2cFuncion;

char  ardI2C[5];		// Chip que envia informacion.
char  strI2C[190];		// Datos. cada vez que van llegando datos se van guardando. debe ser GLOBAL.
byte  chari2c;

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
