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
 
/* ******************************************************************************************
 *                                     VARIABLES DE PROGRAMA                                *
 ********************************************************************************************/
// VARIABLES GLOBALES.

// Control de tareas.
#define tarea_volumenes	1
#define tarea_venta		2

OneWire   ds1(2);			// Pin para iButton1
OneWire   ds2(3);			// Pin para iButton2

// Variables para comunicacion I2C.
byte  trazaI2C = 0;		// Comando.

char  ardI2C[5];		// Chip que envia informacion.
char  strI2C[190];		// Datos. cada vez que van llegando datos se van guardando. debe ser GLOBAL.
byte  chari2c;
char  *tmpstr1;
byte  PPUI2C = 0;

char  DatosI2C[20];

byte  SolicitudVol=0;

// VARIABLES de Autorizacion por iButton.
byte	Autorizacion;
//char	C1_Placa[8];
//char	C2_Placa[8];

unsigned long iB_Tini;

//-------
String    keyStatus="";
byte      Combustible;
byte      uso_iButton=0;

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
	//Serial.print( F("Volumen: ") );     Serial.println( venta[pos].Volumen );
	
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
	
	SerialIgem.begin (9600);
	pinMode(TXE485,OUTPUT);
	pinMode(RXE485,OUTPUT);
	digitalWrite(TXE485,0);
	digitalWrite(RXE485,0);
	
	Serial.println( F("Config WIRE I2C") );
	Wire.begin(ARD_PROTOCOLO);					// Se inicializa la comunicacion I2C para leer memoria EEPROM1024.
	Wire.onReceive(RecibeRequisicionI2C);
	
	pinMode(7,OUTPUT);
	digitalWrite(7,1);
	
	// --------------- RESETEAR DATOS DE LAS DOS MANGUERAS DE ESTA CARA -------------------
	Serial.println( F("---------------------------") );
	print_infoVenta(0);
	Serial.println( F("---------------------------") );
	print_infoVenta(1);
	Serial.println( F("---------------------------") );
	
	// ----------------------------------------------------------------------
	// FIN SETUP
	
	Serial.print( F("iniciando…") ); Serial.println(millis());

	for( int i=0; i<4; i++ )
	{
    unsigned char trama[25];
		int manguera=0;
		int res;
  
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);
		delayx(DELAYWAYNE);
    
		getVenta(IDs[i]);                   // Solicita VENTA.
		delayx(DELAYWAYNE);
    
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)  VerificaRecibido( trama, res);    // */
		delayx(DELAYWAYNE);
    
		getTotales( IDs[i], manguera );             // Solicita TOTALES.
		delayx(DELAYWAYNE);
    
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);    // */
		delayx(DELAYWAYNE);
    
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);
		delayx(DELAYWAYNE);
	}
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
  byte   Venta_En;
	Venta_En = digitalRead(8);  Serial.print(Venta_En);   Serial.print(F(" - "));
	Venta_En = digitalRead(9);  Serial.print(Venta_En);
	Serial.print( F(" ____________________________________________________"));
	Serial.print( F(" - "));    Serial.print(millis());  Serial.print( F(" - ContLoop: "));    Serial.println(ContLoop);

/*  if(ContLoop==10)
  {
    setPrecio( byte ID, byte manguera, 7210 );
  } // */

  
	LoopI2C_Comunicacion();			// Si no llega traza, su uso de tiempo es minimo.
	LoopProtocolo_wayne();
	
	// ------ Verificar si hay se debe desautorizar mangueras ------
	if(( millis()- iB_Tini > ESPERAIBUTTON )&&(Autorizacion!=0))		// 1 o 2.
	{
    Serial.print( F("millis: ") );    Serial.println( millis() );
		Serial.print( F("iB_Tini: ") );    Serial.println( iB_Tini );
		
		Serial.println( F("Deshabilitar mangueras ***") );
		
//		C1_Placa[0] = 0;		C2_Placa[0] = 0;
		Autorizacion = 0;
		//iB_Tini = 0;
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
}
*/
// ------------------
void RecibeRequisicionI2C( int howMany )	// Se mantiene sin informacion la interrupcion.
{
	Serial.print(  F("I2C. howMany: ") );	Serial.println( howMany );
	
	// Leer la traza que se recibe a travez del puerto i2c.
	volatile byte  varI = 0;
	while (Wire.available() > 0)
	{
		char cI2C = Wire.read();
		
		if(varI<188) strI2C[varI++] = cI2C;
	}	strI2C[varI]=0;
	
	Serial.print( F("I2C. ") );		Serial.print(strI2C);
	Serial.print( F("@") );			Serial.println(varI);
	
	// separacion de los elementos de la trama enviada.
	char s2[4] = ":";
	char *ptr;
	//String  REQcomand[25];
	char  REQcomand[25];
	
	ptr = strtok( strI2C, s2 );    // Primera llamada => Primer token
	//Serial.print( F("I2C. inicial: ") );	Serial.println( ptr );
	
	ptr = strtok( NULL, s2 );
	strcpy( ardI2C, ptr );			//Serial.print( F("I2C. arduino: ") );		Serial.println( ardI2C );
	
	ptr = strtok( NULL, s2 );
	strncpy( REQcomand, ptr, 24 );		//Serial.print( F("I2C. comando: ") );		Serial.println( REQcomand );
	
	ptr = strtok( NULL, s2 );
	strncpy( DatosI2C, ptr, 19 );		//Serial.print( F("I2C. datos: ") );		Serial.println( DatosI2C );
	
	// Verificar que el primero si sea "innpetrol"
	trazaI2C = 0;

	Serial.print( F("I2C. Comando: ") );   Serial.print(REQcomand);
  
	if( strcmp_P( REQcomand, (PGM_P)F("numeracion") )==0 )				// Solicitud de enviar valor de ibutton. para buscar en la base de datos.
	{
		Serial.println(F("Llega solicitud de numeracion"));
		SolicitudVol=1;
		trazaI2C = 2;
	}
	
	if( strcmp_P( REQcomand, (PGM_P)F("estado") )==0 )					// Solicitud de releer.
	{
		trazaI2C = 5;
	}
	
	if( strcmp( REQcomand, "newprecio" )==0 )       // Solicitud de enviar valor de Estado del turno.
	{
		Serial.println(F("ORDEN CAMBIAR PRECIOS..."));
    
	//Cargar estructura con datos del precio.
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
    
    // variable aparte para tema de precios.
		PPUI2C = 7;
		trazaI2C = 7;
	} // */
	
  // ____________________________________________________________________________________________________
	if( strcmp( REQcomand, "aut" )==0 )       // Solicitud de enviar valor de Estado del turno.
	{
		Serial.println(F("ORDEN VALORES DE AUTORIZACION..."));
    
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
    
//    	  PPUI2C = 8;
//    	  trazaI2C = 8;
	} // */
	
	if( strcmp_P( REQcomand, (PGM_P)F("S2V") )==0 )
	{
	}
  
	if( strcmp_P( REQcomand, (PGM_P)F("S2T") )==0 )
	{
		return;
	}
  
	//--------------------------------------------------
	if(trazaI2C==0)						// Se recibe una traza que no identifica un comando.
	{
		Serial.print( F("ERROR ***** I2C. Sale de I2C...") );
		Serial.println( trazaI2C );
	}
	else
	{
		Serial.print( F("ORDEN...") );
		Serial.println( trazaI2C );
	}
}	//  */

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
byte  enviarVenta( byte pos )		// cara A o B
{
	int i;
	
	if( pos<=4 )		strcpy( strI2C, F("innpe:A:venta:") );
	else				strcpy( strI2C, F("innpe:B:venta:") );
	
	tmpstr1 = (char*)(&venta[pos]);
	for(i=0; i<sizeof(venta[pos]); i++)
	{
		chari2c = tmpstr1[i];
		
		if( chari2c<16 )	sprintf( strI2C, "%s0%x", strI2C, chari2c );
		else				sprintf( strI2C,  "%s%x", strI2C, chari2c );
	}
	Serial.println(strI2C);		// estructura que contiene la informacion a enviar.
	
	i=0;
	int    intTemp;
	intTemp = strlen(strI2C);
	while( 32*i< intTemp )
	{
		char  bufI2C[35];
    
		strncpy(bufI2C, &strI2C[i*32], 32);
		Serial.println(bufI2C);
		
		Wire.beginTransmission(ARD_MEGA2560);
		Wire.write(bufI2C);
		Wire.endTransmission();
		
		delay(7);
		
		i++;
	}
	
	Serial.println( F("Enviada traza I2C") );
	
	Wire.beginTransmission(ARD_MEGA2560);
	Wire.write("END");
	Wire.endTransmission();
}
