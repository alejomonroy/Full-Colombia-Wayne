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

//byte  PARIDAD;
//#define    PAR     0
//#define   IMPAR   1

#include <SoftwareSerial.h>

#include <Wire.h>            // I2C
#include <avr/pgmspace.h>

#define DELAYWAYNE  50
#define ESPERAIBUTTON	6000

#define GASOLINA  1
#define ACPM      2
#define EXTRA     3

#define ARD_MEGA2560	0x1
#define ARD_PROTOCOLO	0x2
#define ARD_PROTOCOLO2	0x4
#define myADDR 			ARD_PROTOCOLO

#define AUTORIZAR 1
#define PRECIOS   2
#define VENTAS    3
#define ESTADO_M  4
#define TOTALES   5
#define CONFIG    6

/* ******************************************************************************************
 *                                     VARIABLES DE PROGRAMA                                *
 ********************************************************************************************/
// Variables para comunicacion I2C.
char  *tmpstr1;

void getKeyCode(OneWire   ds);   // Leer codigo iButton

// ------------------------------------------------------------------------------------------------------
//    DECLARACION DE FUNCIONES.
byte  enviarVenta( byte pos );

void print_infoVenta( byte varManguera);
void Verificar_iButton(int cara);

void RecibeRequisicionI2C( int howMany );

int ContLoop=0;

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
 *                                   Escribir Estructuras en EEPROM                                  *
 *****************************************************************************************************/
void RD_Struct( unsigned int addr_eeprom, long tamano, char  *tmpstr )
{
	int  i;
	char	tmpEEPROM;
  
	for(i=0; i<tamano; i++)
	{
		tmpEEPROM = EEPROM.read(i + addr_eeprom);
		tmpstr[i] = tmpEEPROM;
	}
}

// ---------------------------------------------------------------
//                        FUNCIONES DE VENTAS
// ---------------------------------------------------------------
void print_infoVenta( byte pos )
{
  char  strVolumen[15];
	
	Serial.print( F("----- POS ----- ") );     Serial.println( pos );
	
	dtostrf( venta[pos].Volumen, 4, 3, strVolumen);
	Serial.print( F("Volumen: ") );     Serial.println( strVolumen );
	Serial.print( F("Venta: ") );     Serial.println( venta[pos].Venta );
	Serial.print( F("PPU: ") );       Serial.println( venta[pos].PPU );
	Serial.print( F("Numeracion: ") );      Serial.println( venta[pos].Numeracion );
}

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
	Wire.onReceive(Recibe_Comm_I2C);
	Wire.onRequest(Request_I2C);
	
	pinMode(7,OUTPUT);
	digitalWrite(7,1);
	
	// --------------- RESETEAR DATOS DE LAS DOS MANGUERAS DE ESTA CARA -------------------
	Serial.println( F("---------------------------") );
	print_infoVenta(0);
	print_infoVenta(1);
	print_infoVenta(2);
	print_infoVenta(3);
	print_infoVenta(4);
	print_infoVenta(5);   // las 6 mangueras.
	Serial.println( F("---------------------------") );
	
	// ----------------------------------------------------------------------
	// FIN SETUP
	
	Serial.print( F("iniciando…") ); Serial.println(millis());

	/*for( int i=0; i<4; i++ )
	{
	    unsigned char trama[25];
		int manguera=0;
		int res;
  
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);
		delay(DELAYWAYNE);
    
		getVenta(IDs[i]);                   // Solicita VENTA.
		delay(DELAYWAYNE);
    
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)  VerificaRecibido( trama, res);
		delay(DELAYWAYNE);
    
		getTotales( IDs[i], manguera );             // Solicita TOTALES.
		delay(DELAYWAYNE);
    
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);
		delay(DELAYWAYNE);
    
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);
		delay(DELAYWAYNE);
	} */
	print_infoVenta(0);
	print_infoVenta(1);
	print_infoVenta(2);
	print_infoVenta(3);
	print_infoVenta(4);
	print_infoVenta(5);   // las 6 mangueras.

  /*  setPrecio( 0x50, 1, 8100 );
	setPrecio( 0x50, 2, 8200 );
	setPrecio( 0x51, 1, 8300 );
	setPrecio( 0x51, 2, 8400 );
  
	setPrecio( 0x52, 1, 8500 );
	setPrecio( 0x53, 1, 8600 ); // */
}

// ------------------------------------------------------------------------------------------------
/*	El programa debe mantenerse en contacto con el surtidor por medio de 
	protocolo de comunicacion WAYNE.
	Mantener contacto revizando los iButton para la comunicacion	*/
void loop()
{
	Serial.print( F(" ____________________________________________________"));
	Serial.print( F(" - "));    Serial.print(millis());  Serial.print( F(" - ContLoop: "));    Serial.println(ContLoop);

	LoopI2C_Comunicacion();			// Si no llega traza, su uso de tiempo es minimo.
	LoopProtocolo_wayne();
	
	// ------ Verificar si hay se debe desautorizar mangueras ------
	if(( millis()- iB_Tini > ESPERAIBUTTON ))		// 1 o 2.
	{
	    Serial.print( F("millis: ") );    Serial.println( millis() );
		Serial.print( F("iB_Tini: ") );    Serial.println( iB_Tini );
		
		Serial.println( F("Deshabilitar mangueras ***") );
		
		//Si se levanta manguera se debe detener este procedimiento!!!   @@@ Cuando se activa manguera autorizacion se pone en 0
		
	}	// */
	// -------------------------------------------------------------
	ContLoop++;
}       // FIN LOOP

/* ***************************************************************************************************
 *                                              iButton                                              *
 *****************************************************************************************************/
void getKeyCode(OneWire   ds, byte *addr)   // Leer codigo iButton
{
	byte present = 0;
	byte data[12];
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
void Recibe_Comm_I2C( int howMany )	// Se mantiene sin informacion la interrupcion.
{
	Serial.print(  F("I2C. howMany: ") );	Serial.println( howMany );
	volatile byte  varI = 0;
	while (Wire.available() > 0)
	{
		char cI2C = Wire.read();
		
		if(varI<188) strI2C[varI++] = cI2C;
	}	strI2C[varI]=0;
	
	Serial.print( F("I2C. ") );		Serial.print(strI2C);	Serial.print( F("@") );			Serial.println(varI);
	
	// separacion de los elementos de la trama enviada.
	char s2[4] = ":";
	char *ptr;
	char  REQcomand[25];
	
	ptr = strtok( strI2C, s2 );
	ptr = strtok( NULL, s2 );
	strcpy( ardI2C, ptr );			//Serial.print( F("I2C. arduino: ") );		Serial.println( ardI2C );
	
	ptr = strtok( NULL, s2 );
	strncpy( REQcomand, ptr, 24 );		//Serial.print( F("I2C. comando: ") );		Serial.println( REQcomand );
	
	ptr = strtok( NULL, s2 );
	strncpy( DatosI2C, ptr, 19 );		//Serial.print( F("I2C. datos: ") );		Serial.println( DatosI2C );
	
	// Verificar que el primero si sea "innpetrol"
	Serial.print( F("I2C. Comando: ") );   Serial.print(REQcomand);
	
	// __________________________________________________
	if( strcmp_P( REQcomand, (PGM_P)F("ventas") )==0 )
	{
		i2cFuncion.funcion = VENTAS;
		i2cFuncion.time = millis() + 100;	// Timeout de 100ms.
	}

	// __________________________________________________
	if( strcmp_P( REQcomand, (PGM_P)F("numeracion") )==0 )
	{
		i2cFuncion.funcion = TOTALES;
		i2cFuncion.time = millis() + 100;

		Serial.println(F("Llega solicitud de numeracion"));
	}
	
	// __________________________________________________
	if( strcmp_P( REQcomand, (PGM_P)F("estado") )==0 )
	{
		i2cFuncion.funcion = ESTADO_M;
		i2cFuncion.time = millis() + 100;
	}
	
	// __________________________________________________
	if( strcmp( REQcomand, "newprecio" )==0 )
	{
		Serial.println(F("ORDEN CAMBIAR PRECIOS..."));
		i2cFuncion.funcion = PRECIOS;
		i2cFuncion.time = millis() + 2000;		// La vigencia del comando.

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
		PPUArray[i2cPrecio.manguera] = i2cPrecio.PPU;
		Serial.print(F("Manguera: "));                Serial.println(i2cPrecio.manguera);
		Serial.print(F("PPU     : "));                Serial.println(i2cPrecio.PPU);
	} // */
	
	// __________________________________________________
	if( strcmp( REQcomand, "aut" )==0 )
	{
		Serial.println(F("ORDEN VALORES DE AUTORIZACION..."));

		i2cFuncion.funcion = AUTORIZAR;
		i2cFuncion.time = millis() + 30000;		// La vigencia de la autorizacion no es mas de 30 segundos.
    
		int size = sizeof(i2cAutoriza);
		char  *tmpstr = (char*)(&i2cAutoriza);
    
		for(int i=0; i<size; i++)
		{
			char chari2c = (char2int( DatosI2C[2*i] )<<4) | char2int( DatosI2C[2*i+1] );
			tmpstr[i] = chari2c;
			Serial.print(0xff&chari2c, HEX); Serial.print(F(" "));
		} Serial.println();

		if(i2cAutoriza.modo == 2)
		i2cAutoriza.cantidad = i2cAutoriza.cantidad*1000;

		// poner en cola de ejecucion.
		Serial.print(F("modo    : "));  Serial.println(i2cAutoriza.modo);
		Serial.print(F("lado    : "));  Serial.println(i2cAutoriza.lado);
		Serial.print(F("mang    : "));  Serial.println(i2cAutoriza.mang);
		Serial.print(F("cantidad: "));  Serial.println(i2cAutoriza.cantidad);
	} // */
	
	//--------------------------------------------------
	if(i2cFuncion.funcion==0)						// Se recibe una traza que no identifica un comando.
	{
		Serial.print( F("ERROR ***** I2C. Sale de I2C...") );
		Serial.println( i2cFuncion.funcion );
	}
	else
	{
		Serial.print( F("ORDEN...") );
		Serial.println( i2cFuncion.funcion );
	}
}

/*
// -----------------------------------------------------------------------------------------------------
byte	strcmpEDS(char *str1, char *str2, int	noChar)							// Retorna 0, o otro valor cuando son diferentes.
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
/*
	- master envia el comando en el cual solicita las ventas, finalmente envia el request por secciones de 32 bytes.
	- el esclavo almacena la funcion con un timeout, si llega el requiest dentro del tiempo establecido, 
		envia la informacion en secciones de 32 bytes.
*/
void Request_I2C()			// QUE PASA SI FUERAN 2 O 3 MANGUERAS??????? @@@@@@
{
	/*
		- Request de ventas.
		- Request de estado de mangueras.
		- Request de totales.
	*/
	if((i2cFuncion.funcion == VENTAS)||(millis() < i2cFuncion.time))
		enviarVentas();

	if((i2cFuncion.funcion == ESTADO_M)||(millis() < i2cFuncion.time))
		enviarEstado();

	if((i2cFuncion.funcion == TOTALES)||(millis() < i2cFuncion.time))
	{
		
	}

//		Wire.write(strI2C);

}

// -----------------------------------------------------------------------------------------------------
byte  enviarVentas()
{
	strcpy( strI2C, F("innpe:1:venta:") );
	
	// -------------------------
	// Los datos binarios los transforma en formato hexagesimal.
	tmpstr1 = (char*)(&venta);
	for(int i=0; i<sizeof(venta); i++)
	{
		chari2c = tmpstr1[i];
		
		if( chari2c<16 )  sprintf( strI2C, "%s0%x", strI2C, chari2c );
		else        sprintf( strI2C,  "%s%x", strI2C, chari2c );
	}
	Serial.println(strI2C);		// estructura que contiene la informacion a enviar. 182 bytes (2020-02-02).
	
	// ENVIAR DATOS AL ARDUINO PRINCIPAL.
	int		i=0;
	int		intTemp = strlen(strI2C);

	while( 32*i< intTemp )
	{
		char  bufI2C[55];
		bzero(bufI2C, sizeof(bufI2C));
	
		strncpy(bufI2C, &strI2C[i*32], 32);
		Serial.println(bufI2C);
		
		Wire.write(bufI2C);
		delay(15);
		i++;
	}
	
	Wire.write("END");
	delay(DELAYWAYNE); // */
}

// -----------------------------------------------------------------------------------------------------
byte  enviarEstado()
{
	if( i2cFuncion.funcion == ESTADO_M )
	{
		Serial.println(F("INICIA ESTADO MANGUERAS..."));
		Serial.print(F("M0: "));		Serial.println(mang_status[0]);
		Serial.print(F("M1: "));		Serial.println(mang_status[1]);
		Serial.print(F("M2: "));		Serial.println(mang_status[2]);
		Serial.print(F("M3: "));		Serial.println(mang_status[3]);
		
		char  strTemp[25];
		if( (strcmp(DatosI2C,"ladoA")==0)&&((mang_status[0]==1)||(mang_status[1]==1)) )                        // el estatus debe ser cero para OK.
			strcpy( strTemp, F("innpe:1:estado:error") );
		else
		{
			if( (strcmp(DatosI2C,"ladoB")==0)&&((mang_status[2]==1)||(mang_status[3]==1)) )                        // el estatus debe ser cero para OK.
				strcpy( strTemp, F("innpe:1:estado:error") );
			else
			{
				if(strcmp(DatosI2C,"ambos")==0)
				{
					Serial.println(F("ambos caras..."));
					if((mang_status[0]==1)||
					   (mang_status[1]==1)||
					   (mang_status[2]==1)||
					   (mang_status[3]==1)
					  )   // el estatus debe ser cero para OK.
						strcpy( strTemp, F("innpe:1:estado:error") );
					else
						strcpy( strTemp, F("innpe:1:estado:oK") );
				}
				else
				{
					strcpy( strTemp, F("innpe:1:estado:oK") );
				}
			}
		}
		
		Serial.println(strTemp);
		
		Wire.write( strTemp );
		delay(7);		// Este delay se debe a que se eliminaron los bloqueos en la libreria twi.
		Wire.write( "END" );
		
		Serial.println(F("FIN ESTADO MANGUERAS..."));
		i2cFuncion.funcion = 0;
	}
}
