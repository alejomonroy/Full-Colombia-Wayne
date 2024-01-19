				
				
/*      
				if(mang_status[pos]==1)
				{
					Serial.println(F("FIN VENTA ******* 1"));
					Serial.print(F("Manguera: "));  Serial.println(pre_mang);
					
					finalVenta=1;
					F_globales[pos] = 0;
					F_ventaOk[pos]=1;
					
					if((0x01&ContLoop)!=0)	ContLoop++;
				}
				if(mang_status[pos]==3)		// leer 3 veces los valores de venta.
				{
					mang_status[pos]=0;
					getVenta(ID);                   // Solicita VENTA
					getTotales( ID, pre_mang );     // Solicita TOTALES.
					return -1;
				}
				getVenta(ID);                   // Solicita VENTA
				getTotales( ID, pre_mang );     // Solicita TOTALES.
				mang_status[pos]++;
				return -1; */

/* 
 * Verificar cuantas mangueras por Cara se van a programar.
 * 
 */

/*		51.31.1.1.0.3.4.1.54.38.11.10. - 8F09
		crc16 RX: 8F09- crc16: trama: 88FA
*/

#define  RS485Transmit HIGH
#define  RS485Receive LOW
#define  TXE485 6
#define  RXE485 5

#define IDLE1	0
#define IDLE2	5
#define READY	2		// @@@@@
#define WORK	4

#define F_ESTADO	0x01
#define F_VENTA		0x02
#define F_TOTAL		0x04
#define F_PRECIO	0x08

/* ***************************************************************************************************
 *                                            FUNCIONES                                              *
 *****************************************************************************************************/
// FUNCIONES DE MENOR NIVEL
long	get_crc_16( unsigned char *buf, int size );					// oK
int		EnviarID(byte ID);											// oK
int		EnviarTrama( byte ID, unsigned char *trama, int	n );		// oK
int		RecibirTrama( unsigned char *trama );						// oK		****
int		CerrarComunicacion(byte ID, byte consecutivo);				// oK
int		VerificaRecibido( unsigned char *trama, int n);

// FUNCIONES
int		getTotales(byte ID, byte manguera);									// oK
int		autorizar(byte ID, byte manguera, byte modo, long cantidad, byte *precioBDC);
int		desautorizar(byte ID);													// oK
int		getVenta( byte ID );													// oK
int		getEstado(byte ID);														// oK

int		setPrecio( byte ID, byte manguera, unsigned int PPU );					// Para despues.

/* ***************************************************************************************************
 *																									 *
 *****************************************************************************************************/
SoftwareSerial SerialIgem(4, 7);    // RX, TX

/* ***************************************************************************************************
 *                                               VENTAS                                              *
 *****************************************************************************************************/
typedef struct          // Dato que se acaba de leer desde el protocolo.
{
	double			Volumen;	// 4
	unsigned long	Venta;		// 4
	unsigned int	PPU;		// 2
	
	unsigned long	Numeracion;		// 4
}Venta;						// 23
Venta   venta[6], venta2[6];    // Se actualiza en cada ciclo.

// -------------------------
typedef struct				// Dato que se acaba de leer desde el protocolo.
{
	byte			manguera;		// 1
	double			Volumen;	// 4
	unsigned long	Venta;	// 4
	unsigned int	PPU;		// 2
}VentaS2;					// 16

// -------------------------
typedef struct				// Dato que se acaba de leer desde el protocolo.
{
	byte			manguera;			// 1
	unsigned long	Numeracion;			// 4
}NumeracionS2;						// 16

// ------------------------------------------------------------------------
typedef struct    // 3 bytes
{
	byte manguera;
	unsigned int PPU;
} I2CPrecio;
unsigned int PPUArray[6]={0, 0, 0, 0, 0, 0};    // 12 bytes

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
 *                                            VARIABLES                                              *
 *****************************************************************************************************/
byte		mang_status[4] = {0 ,0 ,0 ,0};				// por CARA.
byte		IDs[4]= {0x50, 0x51, 0x52, 0x53};

byte		F_ventaOk[4]		= {0, 0, 0, 0};			// por CARA.
byte		F_enviado[4]		= {0, 0, 0, 0};			// por CARA.

byte		F_globales[4]		= {0, 0, 0, 0};			// por CARA.
byte    	UltManguera[4]		= {0, 0, 0, 0};			// por CARA.

int res;
unsigned long	tini_loop, tact_loop;

/* ***************************************************************************************************
 *                              IMPLEMENTACION DE FUNCIONES MENOR NIVEL                              *
 *****************************************************************************************************/
// Primero se envia el byte menos significativo
// despues el byte mas significativo, se calcula con toda la trama, incluyendo el ID de la cara.
byte	crc_error;
long get_crc_16( unsigned char *buf, int size )
{
	int   k;
	unsigned long  ch;
	unsigned long  crc=0;
	
	//Serial.print( F("size: ") );   Serial.println(size);
	while(size--)
	{
		ch = *buf++;     //Serial.print(ch, HEX);    Serial.print(".");
		for (k = 0; k < 8; k++)
		{
			if ((crc ^ ch) & 1)
				crc = (crc >> 1) ^ 0xa001;
			else
				crc >>= 1;
			ch >>= 1;
		}
	}	//Serial.print(" - ");   Serial.println(crc, HEX);
	
	return crc;
}

// ----------------------------------------------------------------------------------------------------
// Medir tiempos medios en recibir respuesta, sacar promedio y esperar maximo doble o triple de estos tiempos.
int		EnviarID( byte ID )
{
	digitalWrite(TXE485,RS485Transmit);  digitalWrite(RXE485,RS485Transmit);
	
	delay(1);
	SerialIgem.write( ID );
	SerialIgem.write(0x20);
	SerialIgem.write(0xfa);
	
	digitalWrite(TXE485,RS485Receive);  digitalWrite(RXE485,RS485Receive);
	
	Serial.print( F("ID: (") );    // Usa: 0.5 ms
	Serial.print( ID, HEX );	Serial.print( F(" 20 fa) ---> ") );	Serial.println( millis() );  // */
	return 0;
}

unsigned char ContRecepcion = 0x30;

// ----------------------------------------------------------------------------------------------------
byte   ContEnvio[4] = {0x30, 0x30, 0x30, 0x30};		// las caras posibles.

int    EnviarTrama( byte ID, unsigned char *trama, int n ) // 51 36 Trama crc 03 FA    // Ejp. | 50 | 33 | 65 1 2 | 62 C6 | 3 FA
{
	int i;
	unsigned int crc;
	int pos = -1;
	
	// ----------------------------------------------------------------
	if(ID == IDs[0]) pos = 0;
	if(ID == IDs[1]) pos = 1;
	if(ID == IDs[2]) pos = 2;
	if(ID == IDs[3]) pos = 3;
	if(pos < 0) return 0;
	
	// ----------------------------------------------------------------
	trama[0] = ID;
	trama[1] = ContEnvio[pos];
	n=n+2;
	
	crc = get_crc_16( trama, n );
	if((0xff&crc)==0xfa) trama[n++] = 0x10;
	trama[n++] = 0xff&crc;
	
	if(0xff&(crc >>8)==0xfa) trama[n++] = 0x10;
		trama[n++] = 0xff&(crc >>8);
	
	digitalWrite(TXE485,RS485Transmit);
	digitalWrite(RXE485,RS485Transmit);
	
	Serial.print( F("TX: (") );
	for(i=0; i<n; i++)
	{
		SerialIgem.write(trama[i]);
		Serial.print(trama[i], HEX);    Serial.print(F(" "));
	}
	
	SerialIgem.write(0x03);
	SerialIgem.write(0xfa);
	
	Serial.println(F("3 fa)"));
	
	while(SerialIgem.available() > 0) SerialIgem.read();  // Borrar datos y posibles errores en puerto serial.
	
	digitalWrite(TXE485,RS485Receive);
	digitalWrite(RXE485,RS485Receive);
	return 0;
}

// ----------------------------------------------------------------------------------------------------
int    RecibirTrama( unsigned char *trama )  // Tiempo de espera = 100ms
{
	int 	pos = 0;
	char	c;
	
//  Serial.println(F("punto 1"));
	// recibir una trama hasta el fin de linea 0xfa
	tini_loop = millis();
	do
	{
		while(SerialIgem.available() > 0)
		{
			c = SerialIgem.read();
			//Serial.println(c, HEX);
			if( pos<99 ) trama[pos++] = c;
			
			tini_loop = millis();
		}trama[pos]=0;
		
		if(pos<4)
		{
			if( (0xff&c) == 0xfa)
			{
				//Serial.println(F("break 1."));
				break;
			}
		}else
		{
			if((trama[pos-2] == 0x03)&&((0xff&c) == 0xfa))
			{
				//Serial.println(F("break 2."));
				break;
			}
		}	// */
	
	}while( (millis()-tini_loop)<50 ); // Terminar si no llega traza y se toma como cara AUSENTE.
	
//  Serial.println(F("punto 2"));
	//Serial.println(F("______"));
	if((millis()-tini_loop)>50)
	{
		Serial.print(pos);
		Serial.println(F(" - Recibir. Time Out..."));
	} // */
	if(pos<1) return 0;
  
	// Recalculo el contador de las tramas recibidas.
	if( ContRecepcion==0 )  ContRecepcion = trama[1];
	else
	{
		ContRecepcion++;
		if(ContRecepcion>0x3f) ContRecepcion = 0x31;
	}
	
	// mostrar la trama recibida.
	Serial.print(F(":"));	Serial.print(tini_loop);	Serial.print(F(", RX2:"));  Serial.print(pos);  Serial.print(F(": "));
	if(pos ==0)   Serial.println(F("NO TRAZA..."));  // Mostros traza que llega.
	else
	{
		
		Serial.print(F("("));
		for(int i=0; i<pos; i++) { Serial.print(trama[i], HEX);  Serial.print( F(" ") ); }
		Serial.print(F(") <--- "));		Serial.println(millis());
	}	//*/

	return pos;   // No se recibio nada.
}

// ----------------------------------------------------------------------------------------------------
int		EnviarTrama2( byte ID, unsigned char *trama, int n )		// Con confirmacion.
{
	unsigned char trama2[10];
	int pos=-1;
	if(ID == IDs[0]) pos = 0;
	if(ID == IDs[1]) pos = 1;
	if(ID == IDs[2]) pos = 2;
	if(ID == IDs[3]) pos = 3;
	if(pos < 0) return 0;
	
	for(int i=0; i<3; i++)
	{
		EnviarTrama(ID, trama, n);        // Envia solicitud de volumen.    51 35 | 1 1 0 | A2 50 | 3 FA
		res = RecibirTrama( trama2 );      // Validar trama recibida.        50 C5 FA
		
		if(res>19)
		{
			Serial.println(F("PELIGRO* SE DESBORDO tramarecv... PELIGRO*****"));
			Serial.println(F("PELIGRO* SE DESBORDO tramarecv... PELIGRO*****"));
			Serial.println(F("PELIGRO* SE DESBORDO tramarecv... PELIGRO*****"));
		}
		if(res>=3)			// No se recibio nada.
		{
			ContEnvio[pos]++;
			if(ContEnvio[pos]>0x3f) ContEnvio[pos] = 0x31;
			if( (trama2[1]&0xf0) == 0xc0 ) return 1;    // Recibido Ok.
			if( (trama2[1]&0xf0) == 0x50 ) ContEnvio[pos] = 0x30;    // Reiniciar el contador de envio en tarjeta FULL y en wayne.
		}
		delay(70);    Serial.print(F("delay - "));    Serial.println(millis());		// En caso de error.
	}
	ContEnvio[pos]++;
	if(ContEnvio[pos]>0x3f) ContEnvio[pos] = 0x31;
	
	return -1;
}

// ----------------------------------------------------------------------------------------------------
int    CerrarComunicacion(byte ID, byte consecutivo)
{
	/*  Surt  50 CE FA  */
	digitalWrite(TXE485,RS485Transmit);
	digitalWrite(RXE485,RS485Transmit);
	
	SerialIgem.write( ID );
	SerialIgem.write( 0xc0 + (0x0f&consecutivo) );
	SerialIgem.write( 0xfa );
	
	digitalWrite(TXE485, RS485Receive);
	digitalWrite(RXE485, RS485Receive);
	
/*	Serial.print(F("Cerrar COM: ("));
	Serial.print( ID, HEX );    Serial.print(" ");
	Serial.print( 0xc0 + (0x0f&consecutivo), HEX);              Serial.print(" ");
	Serial.println(F("fa) --->"));	// */
	return 0;
}

// ----------------------------------------------------------------------------------------------------
// si llega 52 c3 fa (return 1) o 52 53 c3 (return -3)
unsigned long tmpnumeracion = 0;

byte	finalVenta=0;
byte  	validarDatos=0;		// Un ciclo de retraso para que valide datos.

int		VerificaRecibido( unsigned char *trama, int n)
{
	byte	ret=0;
	byte	F_locales = 0;
	
	// Funcion volumenes totales
	byte total_mang = 0;				// cual fue la manguera que llega su total.
	
	// Funcion precios
	byte precioBDC[5] = { 0, 0, 0, 0, 0 };
	unsigned int temp_precio = 0;
	byte pre_mang = 0;
	byte precio_mang_status = 0;
	byte temp_estado = IDLE1;

	// Funcion precios
	unsigned long tempvolumen=0;
	unsigned long tempventa =0;
	
	//VERIFICACION DE CUAL CARA ES.
	int pos=-1;
	byte ID=0;
	unsigned int crc;
	int i=0;
	
	// --------------------------------------------------
	ID = trama[0];
	if(ID == IDs[0]) pos = 0;
	if(ID == IDs[1]) pos = 1;
	if(ID == IDs[2]) pos = 2;
	if(ID == IDs[3]) pos = 3;

	if(pos < 0) return 0;
	
	// --------------------------------------------------
	if(n<3)	return -1;						// No hay trazas de menos de tres bytes.
	if(n==3)								// 53 c4 fa / 53 54 fa	(ok/error)
	{
		if(n ==0)   Serial.println(F("NO TRAZA..."));  // Mostros traza que llega.
		else
		{
//			for(int i=0; i<n; i++) { Serial.print(trama[i], HEX);  Serial.print(":"); }
//			Serial.println();
		}
		
		if(  trama[2] != 0xfa)							return -2;		// Error en el recibido.
		if( (trama[1]) == ((0x0f&ContEnvio[pos])|0x50) )		return -3;		// Error en el recibido.
		if( (trama[1]) == ((0x0f&ContEnvio[pos])|0xc0) )		return 1;		// Recibido Ok.
		return -4;
	}
	
	// ---------- VERIFICACION DE TRAZA RECIBIDA. -----------
	// Consecutivo
	//if(ContRecepcion != trama[1]) return -3;

	if(n >= 9)				// 51 35 1 1 5 da cb 3 fa
	{
		crc = get_crc_16( trama, n-4 );
		Serial.print(F("crc16 RX: "));  Serial.print(crc, HEX);
		Serial.print(F("- crc16: trama: "));  Serial.print((unsigned int)((trama[n-3]<<8) + trama[n-4]), HEX);
		
		if((trama[n-3]!=0xfa)&&(trama[n-4]!=0xfa))
		{
			if(crc != ((trama[n-3]<<8) + trama[n-4]))
			{	// ERROR CRC.
				Serial.println(F("ERROR *********"));
				crc_error++;
				if( crc_error<2 )  return -5;    		// La traza no tiene bien el crc.
				CerrarComunicacion(ID, trama[1]);		// descartar traza. millis
				return -6;
			} Serial.println();
		}
		
		// ------------------------------------------------------
		CerrarComunicacion(ID, trama[1]);          						// Se cierra comunicacion.      50 CE FA
		
		int ciclos_n = 0;
		i = 2;
		while( (n-i)>4 )
		{
			Serial.print(F("----- n: "));	Serial.print(n);	Serial.print(F(" ,i: "));	Serial.println(i);
			if(ciclos_n++>10) return -7;
			
			// --------------------------------------------------
			if((trama[i]==0x01)&&(trama[i+1]==0x01))
			{
				Serial.println(F("     **** Estado ****"));
				
				i+=2;
				temp_estado = trama[i++];
				
				F_locales |= F_ESTADO;			// pone a 1
			}
			
			// --------------------------------------------------
			if((trama[i]==0x02)&&(trama[i+1]==0x08))
			{							//			51 31 1 1 4 3 4 0 81 23 11 2 8 0 0 4 28 0 0 34 77 3  4 0 81 23 11 83 55 3 FA		Venta
				Serial.println(F("     **** Venta ****"));
				i+=2;
				
				// 4 bytes 00 00 04 28 -> 0.428
				long  longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempvolumen  = 1000000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempvolumen += 10000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempvolumen += 100*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0X0f&trama[i++]);	tempvolumen += longTemp;
				
				//	4 bytes 00 00 34 77 -> 3477
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempventa  = 1000000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempventa += 10000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempventa += 100*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0X0f&trama[i++]);	tempventa += longTemp;
				
				F_locales |= F_VENTA;			// pone a 1
			}
			
			// --------------------------------------------------
			if((trama[i]==0x65)&&(trama[i+1]==0x10))
			{							//			51 39 65 10 1 0 0 42 66 33 0 0 42 66 33 0 0 0 0 0 A4 F0 3 FA
				Serial.println(F("     **** Totales ****"));
				i+=2;
				
				total_mang= 0x0f&trama[i++];			// Manguera.
				
				Serial.print(F("VOLUMENES: "));
				for(int j=i; j<i+10; j++)
				{
					Serial.print(trama[j], HEX);
					Serial.print("-");
				}	Serial.println();
				
				long  longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);		tmpnumeracion  = 10000000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);		tmpnumeracion += 100000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);		tmpnumeracion += 1000*longTemp;
				longTemp = (0x0f&(trama[i]>>4)); 							tmpnumeracion += 100*longTemp;
				
				longTemp = 10*(0x0f&trama[i++]) + (0x0f&(trama[i++]>>4));	tmpnumeracion += longTemp;
				
				i += 10;
				F_locales |= F_TOTAL;			// pone a 1
			}
			
			// --------------------------------------------------
			if((trama[i]==0x03)&&(trama[i+1]==0x04))
			{							//			51 3A 3 4 0 81 23 11 26 14 3 FA				// Llega sin hacer solicitud previa
				Serial.println(F("     **** Precio ****"));
				i+=2;
				
				precioBDC[0] = trama[i];
				precioBDC[1] = trama[i+1];
				precioBDC[2] = trama[i+2];
				
				temp_precio  = 100000*(0x0f&(trama[i]>>4)) + 10000*(0x0f&trama[i++]); 	                      //	4 bytes 00 81 23 -> 8123
				temp_precio +=		1000*(0x0f&(trama[i]>>4)) +   100*(0x0f&trama[i++]);
				temp_precio +=		  10*(0x0f&(trama[i]>>4)) +       (0x0f&trama[i++]);
				
				precio_mang_status = 0x0f&(trama[i]>>4);			// Estado Manguera.
				pre_mang = 0x0f&trama[i++];					// Manguera (1, 2, 3).
				
				//validarDatos=1;
				F_locales |= F_PRECIO;			// pone a 1
			}
			// Autorizacion.	Flag Autorizacion.		1 1 1 1 1 2
			/*	51 3D 1 1 1 1 1 2 1 1 2 3 4 0 81 23 11 B1 CE 3 FA:IG		Autorizacion
				51 3E 1 1 1 1 1 2 F7 4F 3 FA
				51 33 1 1 1 1 1 2 2B 8F 3 FA	*/
		}
		
		byte index;
		// ----------------------------------------------------------------------------------------------------
		if(F_locales&F_ESTADO)
		{
			switch(temp_estado)
			{
				case IDLE1:					break;
				case READY:					break;
				case WORK:					break;
				case IDLE2:					break;
				default: break;
			}
			
			F_locales &= ~F_ESTADO;
		}
		
		// ---------------------------------------------------------------------------
		if(F_locales&F_TOTAL)
		{
			//index = 2*pos + total_mang -1;    // Para surtidores con 1 mangueras.
			//index = 1*pos + total_mang -1;    // Para surtidores con 2 mangueras.
			if((pos==0)||(pos==1)) index = 3*pos + total_mang -1;    // Para surtidores con 3 mangueras.
			if(pos==2) index = 2;
			if(pos==3) index = 5;
			
			Serial.print(F("Manguera: "));		Serial.print(total_mang);
			
			Serial.print(F(" ,3* "));			Serial.print(pos);    Serial.print(F(" + "));
			Serial.print(total_mang);			Serial.println(F(" -1"));
			
			Serial.print(tmpnumeracion);
			Serial.print(F(" - index: "));				Serial.println(index);
			
			// datos de totales.
			if(mang_status[pos]==0)
			{
				if((venta[index].Numeracion - tmpnumeracion)!=0) Serial.println(F("NO COINCIDE NUMERACION"));
				
				if(tmpnumeracion !=0) venta[index].Numeracion = tmpnumeracion;
				
				F_globales[pos] |= F_TOTAL;
				F_locales &= ~F_TOTAL;
				
				ret = total_mang;
			}
		}
		
		// ---------------------------------------------------------------------------
		byte bytetemp=0;
		
		Serial.print(F("12 ID: "));   Serial.print(ID, HEX);  Serial.print(F(", pos: "));  Serial.println(pos);
		if(F_locales&F_PRECIO)
		{
			//index = 2*pos + pre_mang -1;		// CONVERTIR EN VARIABLE DE SISTEMA.
			//index = 1*pos + pre_mang -1;			// NO FUNCIONA EN FUSAGASUGA
			if((pos==0)||(pos==1)) index = 3*pos + pre_mang -1;    // Para surtidores con 3 mangueras.
			if(pos==2) index = 2;
			if(pos==3) index = 5;
			
			Serial.print(F("F_PRECIO| "));
			Serial.print(F("Manguera: "));      Serial.print(pre_mang);     
			Serial.print(F(", Precio: "));			Serial.print(temp_precio);
			Serial.print(F(", Est Ant: "));			Serial.print(mang_status[pos]);
			Serial.print(F(", Estado: "));      Serial.print(precio_mang_status);
			Serial.print(F(", index: "));      Serial.println(index);
			
			if(temp_precio != 0)										// Si el precio en diferente de cero se actualiza.
			{
				UltManguera[pos] = pre_mang;
				venta[index].PPU = temp_precio;
       
				F_globales[pos] |= F_PRECIO;
			}
			//if(pre_mang != 0) 	venta[index].Manguera = pre_mang;
			
			// -------------------------
			if( (mang_status[pos]==0)&&(precio_mang_status==1) )		// INICIO DE LA VENTA	***
			{
				//desautorizar(ID);
				delay(100);
				
				Serial.println(F("INICIA VENTA *******"));
				Serial.print(F("ID: "));  Serial.println(ID, HEX);
				
				//			AUTORIZACION
				autorizar(ID, pre_mang, 0, 0, precioBDC);
				
				mang_status[pos]=1;
				F_ventaOk[pos] = 0;
				F_globales[pos] = 0;
			}
			
			/*
				Si esta manguera levantada y autorizacion 1 o 2, debe autorizar manguera nuevamente.
			*/
			// -------------------------
			if( (mang_status[pos]==1)&&(precio_mang_status==0) )		// FINAL DE LA VENTA	***
			{
				Serial.println(F("FIN VENTA ******* 1"));
				validarDatos=1;
				
				Serial.print(F("Manguera: "));  Serial.println(pre_mang);
				
				finalVenta=1;
				F_globales[pos] = 0;
				F_ventaOk[pos]=1;
				
				mang_status[pos]=0;
				getVenta(ID);                   // Solicita VENTA
				//getTotales( ID, pre_mang );     // Solicita TOTALES.
				return -1;
			}
			
			F_locales &= ~F_PRECIO;
			ret = pre_mang;
		}
		
		// ---------------------------------------------------------------------------
		if((F_locales&F_VENTA)&&(validarDatos == 1))
		{
			Serial.println(F("LLEGA VENTA 260081V"));
			delay(600);
      
			validarDatos=0;
			getVenta(ID);                   // Solicita VENTA
			getTotales( ID, pre_mang );     // Solicita TOTALES.
			return -1;
		}
		
		if((F_locales&F_VENTA)&&(validarDatos == 0))
		{
			Serial.print(F("F_VENTA | "));
			Serial.print(F("Venta: "));    Serial.print(tempventa);
			Serial.print(F(" ,Volumen: "));   Serial.print(((float)tempvolumen)/1000);
			Serial.print(F(", index:"));   Serial.print(index);			Serial.print(F(" - "));   Serial.println(millis());
			
			// @@@@@ aqui se debe revizar un posible error en los datos que se insertan en venta
			if(precio_mang_status==0)	// actualiza venta solo si la manguera esta colgada. hay que mirar lo de los totales...
			{
				venta[index].Venta = tempventa;
				venta[index].Volumen = ((double)tempvolumen)/1000;
				
				if(finalVenta==1)
				{
					Serial.println(F("FIN VENTA ******* 2"));
					//desautorizar(ID);
					
					//if(venta[index].Venta==0)
					if(tempventa==0)
					{
						Serial.println(F("VENTA EN CEROS *******"));
						//desautorizar(ID);
					}
					
//					C1_Placa[0]=0;	C2_Placa[0]=0;
					
					finalVenta=0;
				}
			}
			
			F_globales[pos] |= F_VENTA;
			F_locales &= ~F_VENTA;
		}
		
		byte  puta = (byte)F_VENTA|F_PRECIO|F_TOTAL;
		// ***************************************************************************
		if( (F_ventaOk[pos]==1) && (F_globales[pos] == puta) )
		{
			Serial.print(F("Index - "));			Serial.println(index);
			// datos de la venta
			Serial.print(F("VENTA NUEVA *************** pos: "));       Serial.println(pos);
			Serial.print(F("Volumen: "));	Serial.println(venta[index].Volumen);
			Serial.print(F("Venta  : "));	Serial.println(venta[index].Venta);
			Serial.print(F("PPU    : "));	Serial.println(venta[index].PPU);
			Serial.print(F("numeracion : "));	Serial.println(venta[index].Numeracion);
			
			long errorVenta = venta[index].Venta - (venta[index].Volumen*venta[index].PPU);
			Serial.print(F("error : "));	Serial.println(errorVenta);
			
			if(( errorVenta<-50)||(50<errorVenta))
			{
				mang_status[pos]=1;
				
				F_ventaOk[pos] = 0;			// No se envia esta venta y se permite releer en el proximo ciclo.
				F_globales[pos] = 0;
				
				return -6;
			}
			
			F_enviado[pos] = 0;
			F_ventaOk[pos]= 0x10 + index;    // 0-7
			Serial.print(F("Volumen 2: ")); Serial.println(venta[index].Volumen);
			
			return ret;
		}
		return ret;
	}
}

/* ***************************************************************************************************
 *                              IMPLEMENTACION DE FUNCIONES MAYOR NIVEL                              *
 *****************************************************************************************************/
int    getEstado(byte ID)         // Estado de cada cara del surtidor.
{
	int res;
	unsigned char trama[15];
	unsigned int crc;
	
	trama[2] = 0x01;    // **** Envio Solcitud de Volumen **** ARMAR TRAZA.
	trama[3] = 0x01;
	trama[4] = 0x00;
	
	EnviarTrama2(ID, trama, 3);        // Envia solicitud de volumen.    51 35 | 1 1 0 | A2 50 | 3 FA
	
	return 0;	// No se recibio nada
}

// ----------------------------------------------------------------------------------------------------
int    getTotales(byte ID, byte manguera)
{
	//  Imp   50 32 | 65 1 1 | 23 3B | 3 FA
	int res;
	unsigned char trama[15];
	unsigned int crc;
	
	trama[2] = 0x65;
	trama[3] = 0x01;
	trama[4] = manguera;    // 1, 2 o 3
	
	EnviarTrama2(ID, trama, 3);        // Envia solicitud de volumen.    51 35 | 1 1 0 | A2 50 | 3 FA
	
	return 0;
}

// ----------------------------------------------------------------------------------------------------
int getVenta( byte ID )   // Llenar una estructura con la informacion de la venta
{
	//  51 3E 1 1 4 A1 B7 3 FA
	int res;
	unsigned char trama[15];
	unsigned int crc=0;
	
	trama[2] = 0x01;
	trama[3] = 0x01;
	trama[4] = 0x04;
	
	EnviarTrama2(ID, trama, 3);        // Envia solicitud de volumen.    51 35 | 1 1 0 | A2 50 | 3 FA
	
	return 0;
}

// ----------------------------------------------------------------------------------------------------
int		autorizar(byte ID, byte manguera, byte modo, long cantidad, byte *precioBDC)			// Por ahora solo autoriza a cualquier monto.
{
	Serial.println();
	Serial.println();
	Serial.println();
	Serial.println(F("Autorizar Venta *******"));
	Serial.print(F("modo    : "));  Serial.println(modo);
	Serial.print(F("mang    : "));  Serial.println(manguera);
	
	// probar si la manguera es la misma, sino, retorna.
	Serial.println(F("--- i2cAutoriza ---"));
	Serial.print(F("modo    : "));  Serial.println(i2cAutoriza.modo);
	Serial.print(F("lado    : "));  Serial.println(i2cAutoriza.lado);
	Serial.print(F("mang    : "));  Serial.println(i2cAutoriza.mang);
	Serial.print(F("cantidad: "));  Serial.println(i2cAutoriza.cantidad);
   
	if((manguera-1) != i2cAutoriza.mang)
	{
		Serial.println(F("*** Manguera DIFERENTE a la seleccionada ***"));
		return -1;
	}

	Serial.println(F("--- verifica lado ---"));
	Serial.print(F("ID1    : "));  Serial.println(IDs[i2cAutoriza.lado], HEX);
	Serial.print(F("ID2    : "));  Serial.println(ID, HEX);
	Serial.println();
  
	if( IDs[i2cAutoriza.lado] != ID )
	{
		Serial.println(F("Numero de lado o cara no corresponde a la autorizada"));
		return -1;
	}
  
	int res = 0;
	unsigned char trama[20];
	unsigned int crc;
	
	trama[2] = 0x02;
	trama[3] = 0x01;
	trama[4] = manguera;
	EnviarTrama2(ID, trama, 3);        // 51 32 2 1 1 92 E4 3 fa
	delay(80);
	
	trama[2] = 0x01;
	trama[3] = 0x01;
	trama[4] = 0x05;
	EnviarTrama2(ID, trama, 3);        // 51 33 1 1 5 62 DB 3 fa
	delay(80);
	
	trama[2] = 0x02;
	trama[3] = 0x01;
	trama[4] = manguera;
	EnviarTrama2(ID, trama, 3);        // 51 34 2 1 1 92 6C 3 fa
	delay(80);
	
	switch(i2cAutoriza.modo)
	{
		case 0:                             // FULL
			i2cAutoriza.cantidad = 999900;
			trama[2] = 0x03;                   // volumen. maximo 999.9 galones
			break;
		case 1:
			trama[2] = 0x04;     break;  // Dinero
    
		case 2: 
			trama[2] = 0x03;     break;  // Volumen.   
	}
  
	trama[3] = 0x04;
  
//  trama[4] = 0x00;
  
  // calcular el valor.
	long ventaBin = i2cAutoriza.cantidad;
	byte ventaBDC[8];
	
	ventaBDC[0] = 0x0f&((ventaBin%100000000)/10000000);
	ventaBDC[1] =   0x0f&((ventaBin%10000000)/1000000);
  
	ventaBDC[2] = 0x0f&((ventaBin%1000000)/100000);
	ventaBDC[3] =   0x0f&((ventaBin%100000)/10000);
	ventaBDC[4] =     0x0f&((ventaBin%10000)/1000);
	ventaBDC[5] =       0x0f&((ventaBin%1000)/100);
	ventaBDC[6] =         0x0f&((ventaBin%100)/10);
	ventaBDC[7] =           0x0f&(ventaBin%10);

	trama[4] = (0xf0&(ventaBDC[0]<<4)) | (0x0f&ventaBDC[1]); // MSB
	trama[5] = (0xf0&(ventaBDC[2]<<4)) | (0x0f&ventaBDC[3]); // MSB
	trama[6] = (0xf0&(ventaBDC[4]<<4)) | (0x0f&ventaBDC[5]);
	trama[7] = (0xf0&(ventaBDC[6]<<4)) | (0x0f&ventaBDC[7]); // LSB

	EnviarTrama2(ID, trama, 6);        // 51 35 3 4 99 99 99 99 4 89 3 fa
	delay(80);  // */

  // __________________________________________________
	trama[2] = 0x01;
	trama[3] = 0x01;
	trama[4] = 0x06;
	EnviarTrama2(ID, trama, 3);        // 51 36 1 1 6 22 16 3 fa
	delay(80);  // */
	return 0;
}

// ----------------------------------------------------------------------------------------------------
int    desautorizar(byte ID)
{
	//  51 3E | 1 1 8 | A1 B2 | 3 FA  : 51 CONT  1 1 8 CRC 3 FA
	//  51 CE FA
	unsigned char trama[11];
	unsigned int crc;
	
	trama[2] = 0x01;
	trama[3] = 0x01;
	trama[4] = 0x08;
	EnviarTrama2(ID, trama, 3);        // 51 36 1 1 8 22 16 3 fa
	
	return 0;
}

// ----------------------------------------------------------------------------------------------------
// en donde guardar los precios de wayne.
unsigned int PPUs[12];

int		setPrecio( byte ID, byte manguera, unsigned int PPU )		// mierda
{
	Serial.print(F("ID: "));  Serial.print(ID, HEX);
	Serial.print(F(", Mang: "));  Serial.print(manguera);
	Serial.print(F(", precio: "));  Serial.println(PPU);
	// 51 3F 5 3 0 81 23 A8 7 3 FA						Una manguera
	// 52 3C 5 9 0 12 34  0 56 78 0 26 53 A9 46 3 FA	Tres mangueras
	unsigned char trama[21];
	unsigned int crc;

	for(int i=0; i<sizeof(trama); i++) trama[i]=0;
	trama[2] = 0x05;
  
	if(manguera==1) trama[3] = 0x03   ;// si manguera es 1.
	if(manguera==2) trama[3] = 0x06   ;// si manguera es 2.
	if(manguera==3) trama[3] = 0x09   ;// si manguera es 3.
  
	int cara=-1;
	if(ID==IDs[0])  cara=0;
	if(ID==IDs[1])  cara=1;
	if(ID==IDs[2])  cara=2;
	if(ID==IDs[3])  cara=3;
  
	if(cara==-1) return -1;
  
	PPUs[3*cara + manguera -1] = PPU;  // almacenar el precio que se quiere cambiar.
  // guardar esta informacion en la E2PROM.
  
	Serial.print(F("i: "));    Serial.print( cara );
	Serial.print(F(", manguera: "));    Serial.print( manguera );
	Serial.print(F(", i PPUs: "));    Serial.println(cara*3 + manguera -1);
  
	for(int i=1; i<=manguera; i++)
	{
		byte  precioBDC[6];
		byte j =3*cara + i -1;
    
		precioBDC[0] = 0x0f&((PPUs[j]%1000000)/100000);
		precioBDC[1] =   0x0f&((PPUs[j]%100000)/10000);
		precioBDC[2] =     0x0f&((PPUs[j]%10000)/1000);
		precioBDC[3] =       0x0f&((PPUs[j]%1000)/100);
		precioBDC[4] =         0x0f&((PPUs[j]%100)/10);
		precioBDC[5] =           0x0f&(PPUs[j]%10);
    
		for(byte k=0; k<6; k++)
		{
			Serial.print(precioBDC[k], HEX);      Serial.print(F(" "));
		}      Serial.println(PPUs[j]);
    
		trama[3*i+1] = (0xf0&(precioBDC[0]<<4)) | (0x0f&precioBDC[1]); // MSB
		trama[3*i+2] = (0xf0&(precioBDC[2]<<4)) | (0x0f&precioBDC[3]);
		trama[3*i+3] = (0xf0&(precioBDC[4]<<4)) | (0x0f&precioBDC[5]); // LSB
    
	}
  
	if(manguera==1) EnviarTrama2(ID, trama, 5);     // si manguera es 1.
	if(manguera==2) EnviarTrama2(ID, trama, 8);     // si manguera es 2.
	if(manguera==3) EnviarTrama2(ID, trama, 11);    // si manguera es 3.  */
	delay(70);
  
	trama[2] = 0x02;
	trama[3] = 0x01;
	trama[4] = manguera;
	EnviarTrama2(ID, trama, 3);        // 51 32 2 1 1 92 E4 3 fa
	delay(80);
  
	trama[2] = 0x01;
	trama[3] = 0x01;
	trama[4] = 0x05;
	EnviarTrama2(ID, trama, 3);        // 51 36 1 1 8 22 16 3 fa
	delay(80);
  
	trama[2] = 0x01;
	trama[3] = 0x01;
	trama[4] = 0x08;
	EnviarTrama2(ID, trama, 3);       // 51 36 1 1 8 22 16 3 fa 

   /*   trama[2] = 0x01;               // AUTORIZAR DESPUES DE ENVIAR PRECIOS
      trama[3] = 0x01;                
      trama[4] = 0x06;
      EnviarTrama2(ID, trama, 3);        // 51 36 1 1 6 22 16 3 fa
      delay(80);

        desautorizar(ID);            // DESAUTORIZAR DESPUES DE ENVIAR PRECIOS */
        
  
	return 0;
	
}
