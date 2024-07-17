/*
 * Antes de compilar verificar el bit de paridad, para protocolo wayne debe ser IMPAR.
 * esto se arregla en el archivo SoftwareSerial.cpp.
 *
 * la libreria Print.cpp se debe quitar las lineas:
 * 	//digitalWrite(22,0);         //delay(1);
 * 
 * La libreria twi.c poner salida de emergencia a cada while que pueda bloquear los microcontroladores.
 */

#include "Arduino.h"
#include <OneWire.h>
#include <EEPROM.h>

//uint8_t  PARIDAD;
//#define    PAR     0
//#define   IMPAR   1

#include <SoftwareSerial.h>

#include <Wire.h>            // I2C
#include <avr/pgmspace.h>

#define DELAYWAYNE  5
#define ESPERAIBUTTON	6000

#define GASOLINA  1
#define ACPM      2
#define EXTRA     3

#define ARD_MEGA2560	0x1
#define ARD_PROTOCOLO	0x2
#define ARD_PROTOCOLO2	0x4
#define myADDR 			ARD_PROTOCOLO

#define AUTORIZAR	1
#define PRECIOS		2
#define VENTAS		3
#define ESTADO_M	4
#define NUMERACION	5
#define SEND_NUM	6
#define CONFIG		7

/* ******************************************************************************************
 *                                     VARIABLES DE PROGRAMA                                *
 ********************************************************************************************/
void getKeyCode(OneWire   ds);   // Leer codigo iButton

// ------------------------------------------------------------------------------------------------------
//    DECLARACION DE FUNCIONES.
uint8_t  enviarVenta( uint8_t pos );

void print_infoVenta(uint8_t surtidor, uint8_t lado);
void Verificar_iButton(int cara);

void RecibeRequisicionI2C( int howMany );


void strcpy(char* buf, const __FlashStringHelper *ifsh)
{
	const char PROGMEM *p = (const char PROGMEM *)ifsh;
	int i = 0;
	uint8_t c  = 0;
	do
	{
		c = pgm_read_byte(p++);
		buf[i++] = c;
	} while ( c != 0 );
}

#include "Variables.h"
#include "Protocolo_Wayne.h"
#include "ProMini_Protocolo.h"

/* ***************************************************************************************************
 *                                        Programa Principal                                         *
 *****************************************************************************************************/
void setup()
{
	Serial.begin (115200);   // debugging
	Serial.println( F("***************************") );
	Serial.println( F("*     REINICIA CHIP       *") );
	Serial.println( F("***************************") );

//  if(PARIDAD==IMPAR)  Serial.println( F("!!!    IMPAR    !!!") );
//  if(PARIDAD==PAR)    Serial.println( F("!!!    PAR    !!!") );

	pinMode(8, INPUT);		digitalWrite(8, 1);
	pinMode(9, INPUT);		digitalWrite(9, 1);
	
	SerialIgem.begin(9600);
	pinMode(TXE485,OUTPUT);
	pinMode(RXE485,OUTPUT);
	digitalWrite(TXE485,0);
	digitalWrite(RXE485,0);
	
	Wire.begin(ARD_PROTOCOLO);
	Wire.onReceive(Recibe_I2C);
	Wire.onRequest(Request_I2C);
	
	pinMode(7,OUTPUT);
	digitalWrite(7,1);
	
	// --------------- RESETEAR DATOS DE LAS DOS MANGUERAS DE ESTA CARA -------------------
	Serial.println( F("---------------------------") );
	print_infoVenta(0, 0);
	print_infoVenta(0, 1);
	print_infoVenta(1, 0);
	print_infoVenta(1, 1);
	print_infoVenta(2, 0);
	print_infoVenta(2, 1);   // los 6 lados.
	Serial.println( F("---------------------------") );
	
	// FIN SETUP
}

// ------------------------------------------------------------------------------------------------
int ContLoop=0;
unsigned long TLoop=0;

void loop()
{
	Serial.print( F("______________________________"));    Serial.print(millis() - TLoop);   
	Serial.print( F(" - "));    Serial.print(millis());  Serial.print( F(" - ContLoop: "));    Serial.println(ContLoop);
	TLoop = millis();
  
	LoopI2C_Comunicacion();			// Si no llega traza, su uso de tiempo es minimo.
	LoopProtocolo_wayne();
	ContLoop++;
}

/* ***************************************************************************************************
 *                                              iButton                                              *
 *****************************************************************************************************/
void getKeyCode(OneWire   ds, uint8_t *addr)   // Leer codigo iButton
{
	uint8_t present = 0;
	uint8_t data[12];
	keyStatus=F("");
	
	if ( !ds.search(addr))
	{
    ds.reset_search();
		return;
	}

	if ( OneWire::crc8( addr, 7) != addr[7])
	{
		keyStatus=  F("CRC invalid");
		return;
	}
	
	keyStatus=F("ok");
	ds.reset();
}

/* ***************************************************************************************************
 *                                         Comunicacion I2C                                          *
 *****************************************************************************************************/
/*char char2int(char input)
{
	if( ('0'<=input) && (input<='9') )  return (input-'0');
	if( ('a'<=input) && (input<='f') )  return (input-'a'+10);
	return 0;
}*/

// ------------------
void Recibe_I2C( int howMany )	// Se mantiene sin informacion la interrupcion.
{
	//Serial.print(  F("Recibe_I2C(). howMany: ") );	Serial.println( howMany );
	char		strI2C[100];		// Datos. cada vez que van llegando datos se van guardando. debe ser GLOBAL.
	volatile	uint8_t  varI = 0;

	while (Wire.available() > 0)
	{
		char cI2C = Wire.read();
		
		if(varI<99) strI2C[varI++] = cI2C;
	}	strI2C[varI]=0;
	
	//Serial.print( F("I2C. ") );		Serial.print(strI2C);	Serial.print( F("@") );			Serial.println(varI);
	
	// separacion de los elementos de la trama enviada.
	char s2[4] = ":";
	char *ptr;
	char  REQcomand[25];
	
	char  ardI2C[5];		// Chip que envia informacion.

	ptr = strtok( strI2C, s2 );
	ptr = strtok( NULL, s2 );
	strcpy( ardI2C, ptr );			//Serial.print( F("I2C. arduino: ") );		Serial.println( ardI2C );
	
	ptr = strtok( NULL, s2 );
	strncpy( REQcomand, ptr, 24 );		//Serial.print( F("I2C. comando: ") );		Serial.println( REQcomand );
	
	ptr = strtok( NULL, s2 );
	strncpy( DatosI2C, ptr, 19 );		//Serial.print( F("I2C. datos: ") );		Serial.println( DatosI2C );
	
	// Verificar que el primero si sea "innpetrol"
	Serial.print( F("I2C. Comando: ") );   Serial.println(REQcomand);
	
	// __________________________________________________
	if( strcmp_P( REQcomand, (PGM_P)F("ventas") )==0 )						// *** SE LLENA LA ESTRUCTURA Y SE TIENE LISTA PARA ENVIAR LOS DATOS SOLICITADOS ***
	{
		i2cFuncion.funcion = VENTAS;
		i2cFuncion.time = millis() + 2000;	// Timeout de 100ms.

		txData[0] = 0;														// Los datos binarios los transforma en formato hexagesimal.
		char  *tmpstr1;
		tmpstr1 = (char*)(&venta);
		for(int i=0; i<sizeof(venta); i++)
		{
			int chari2c = 0xff&tmpstr1[i];
			
			if( chari2c<16 )  sprintf( txData, "%s0%x", txData, chari2c );
			else        sprintf( txData,  "%s%x", txData, chari2c );
		}
		Serial.println(txData);												// estructura que contiene la informacion a enviar. 182 uint8_ts (2020-02-02).*/

		bytesWrite = 0;														// inicia envio de datos desde la posicion 0 del arreglo.
	}

	// __________________________________________________
	if( strcmp_P( REQcomand, (PGM_P)F("numeracion") )==0 )
	{
		Serial.println(F("Llega solicitud de leer numeracion"));

		i2cFuncion.funcion = NUMERACION;
		i2cFuncion.time = millis() + 2000;
	}
	
	// __________________________________________________
	if( strcmp_P( REQcomand, (PGM_P)F("sendNum") )==0 )		// REQUEST.
	{
		Serial.println(F("Llega solicitud de enviar numeracion"));

		i2cFuncion.funcion = SEND_NUM;
		i2cFuncion.time = millis() + 2000;

		txData[0] = 0;														// Los datos binarios los transforma en formato hexagesimal.
		char  *tmpstr1;
		tmpstr1 = (char*)(&numeracion);
		for(int i=0; i<sizeof(numeracion); i++)
		{
			int chari2c = 0xff&tmpstr1[i];
			if( chari2c<16 )  sprintf( txData, "%s0%x", txData, chari2c );
			else        sprintf( txData,  "%s%x", txData, chari2c );
		}
		Serial.print(strlen(txData));		Serial.print(F(": "));		Serial.println(txData);		// estructura que contiene la informacion a enviar. 182 uint8_ts (2020-02-02).*/

		bytesWrite = 0;														// inicia envio de datos desde la posicion 0 del arreglo.
	}
	
	// __________________________________________________
	if( strcmp_P( REQcomand, (PGM_P)F("estado") )==0 )		// REQUEST.
	{
		//Serial.println(F("Estado de mangueras"));

		SYNC = 0;
    //SYNC = millis();

		i2cFuncion.funcion = ESTADO_M;
		i2cFuncion.time = millis() + 200;

		txData[0] = 0;														// Los datos binarios los transforma en formato hexagesimal.
		char  *tmpstr1;
		tmpstr1 = (char*)(&mang_status);
		for(int i=0; i<sizeof(mang_status); i++)
		{
			int chari2c = 0xff&tmpstr1[i];
			if( chari2c<16 )  sprintf( txData, "%s0%x", txData, chari2c );
			else        sprintf( txData,  "%s%x", txData, chari2c );
		}
		Serial.print(strlen(txData));		Serial.print(F(": "));		Serial.println(txData);		// estructura que contiene la informacion a enviar. 182 uint8_ts (2020-02-02).*/

		bytesWrite = 0;														// inicia envio de datos desde la posicion 0 del arreglo.
	}
	
	// __________________________________________________
	if( strcmp( REQcomand, "newprecio" )==0 )
	{
		Serial.println(F("ORDEN CAMBIAR PRECIOS..."));
		i2cFuncion.funcion = PRECIOS;
		i2cFuncion.time = millis() + 1000;		// La vigencia del comando.

		I2CPrecio i2cPrecio;
		int size = sizeof(i2cPrecio);
		char  *tmpstr = (char*)(&i2cPrecio);

		for(int i=0; i<size; i++)
		{
			char chari2c = (char2int( DatosI2C[2*i] )<<4) | char2int( DatosI2C[2*i+1] );
			tmpstr[i] = chari2c;
			Serial.print(0xff&chari2c, HEX); Serial.print(F(" "));
		} Serial.println();

	    // poner en cola de ejecucion.
		uint8_t		surt = i2cPrecio.surtidor;
		uint8_t		lado = i2cPrecio.lado;
		uint8_t		mang = i2cPrecio.manguera;

		PPUArray[surt][lado][mang] = i2cPrecio.PPU;

		Serial.print(F("Surtidor: "));		Serial.println(i2cPrecio.surtidor);
		Serial.print(F("Lado: "));			Serial.println(i2cPrecio.lado);
		Serial.print(F("Manguera: "));		Serial.println(i2cPrecio.manguera);
		Serial.print(F("PPU     : "));		Serial.println(i2cPrecio.PPU);
	} // */
	
	// __________________________________________________
	if( strcmp( REQcomand, "aut" )==0 )
	{
		Serial.println(F("ORDEN VALORES DE AUTORIZACION..."));
//		i2cFuncion.funcion = AUTORIZAR;
//		i2cFuncion.time = millis() + 30000;		// La vigencia de la autorizacion no es mas de 30 segundos.
    
		int size = sizeof(i2cAutoriza);
		char  *tmpstr = (char*)(&i2cAutoriza);
    
		for(int i=0; i<size; i++)
		{
			char chari2c = (char2int( DatosI2C[2*i] )<<4) | char2int( DatosI2C[2*i+1] );
			tmpstr[i] = chari2c;
			Serial.print(0xff&chari2c, HEX); Serial.print(F(" "));
		} Serial.println();

		if(i2cAutoriza.modo == 2)
			i2cAutoriza.cantidad = i2cAutoriza.cantidad*1000;				// en modo de volumen.

		// poner en cola de ejecucion.
		int surt = i2cAutoriza.surtidor;
		int lado = i2cAutoriza.lado;
		Serial.print(F("modo    : "));  Serial.println(i2cAutoriza.modo);
		Serial.print(F("Surtidor: "));  Serial.println(surt);
		Serial.print(F("lado    : "));  Serial.println(lado);
		Serial.print(F("mang    : "));  Serial.println(i2cAutoriza.mang);
		Serial.print(F("cantidad: "));  Serial.println(i2cAutoriza.cantidad);

		funAuth[surt][lado].funcion = AUTORIZAR;
		funAuth[surt][lado].time = millis() + 20000;    // La vigencia de la autorizacion no es mas de 30 segundos.
		
		funAuth[surt][lado].mang	= i2cAutoriza.mang;
		funAuth[surt][lado].modo	= i2cAutoriza.modo;
		funAuth[surt][lado].cantidad = i2cAutoriza.cantidad;

		mang_status[surt][lado] = READY;
	} // */
	
	//--------------------------------------------------
	if(i2cFuncion.funcion==0)						// Se recibe una traza que no identifica un comando.
	{
		Serial.print( F("ERROR ***** I2C. Sale de I2C...") );
		Serial.println( i2cFuncion.funcion );
	}
	else
	{
		Serial.print( F("ORDEN...") );		Serial.print( i2cFuncion.funcion );
    Serial.print( F(" - ") );    Serial.println(millis());
	}
}

/*
// -----------------------------------------------------------------------------------------------------
uint8_t	strcmpEDS(char *str1, char *str2, int	noChar)							// Retorna 0, o otro valor cuando son diferentes.
{
	int		i;
	
	for(i=0; i<noChar; i++)
	{
		if( str1[i]!=str2[i] ) return (i+1);
	}
	return 0;
}
*/
// -----------------------------------------------------------------------------------------------------
void Request_I2C()
{
	int bytesReq = min(32, strlen(txData) - bytesWrite);
	//Serial.print(F("Request_I2C(): "));	Serial.print(bytesReq);		Serial.print(F(" | "));	Serial.print(i2cFuncion.time-millis());
  //Serial.print(F(" | "));  Serial.println(i2cFuncion.funcion);

	if((i2cFuncion.funcion == VENTAS)&&(millis() < i2cFuncion.time))
	{
		//Serial.print(F("Request VENTAS: "));		Serial.println(bytesWrite);
		char  bufI2C[33];
		bzero(bufI2C, sizeof(bufI2C));

		strncpy(bufI2C, &txData[bytesWrite], bytesReq);
		Serial.println(bufI2C);
		
		bytesWrite += Wire.write(bufI2C);
	}

	if((i2cFuncion.funcion == ESTADO_M)&&(millis() < i2cFuncion.time))
	{
		//Serial.print(F("Request ESTADO_M: "));		Serial.println(bytesWrite);
		char  bufI2C[33];
		bzero(bufI2C, sizeof(bufI2C));

		strncpy(bufI2C, &txData[bytesWrite], bytesReq);
		Serial.println(bufI2C);
		
		bytesWrite += Wire.write(bufI2C);
	}

	if((i2cFuncion.funcion == SEND_NUM)&&(millis() < i2cFuncion.time))
	{
		//Serial.print(F("Request SEND_NUM: "));		Serial.println(bytesWrite);
		char  bufI2C[33];
		bzero(bufI2C, sizeof(bufI2C));

		strncpy(bufI2C, &txData[bytesWrite], bytesReq);
		Serial.println(bufI2C);
		
		bytesWrite += Wire.write(bufI2C);
	}
}
