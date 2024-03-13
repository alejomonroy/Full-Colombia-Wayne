#define  RS485Transmit HIGH
#define  RS485Receive LOW
#define  TXE485 6
#define  RXE485 5

#define  IDLE1	0
#define  IDLE2	5
#define  READY	2		// @@@@@
#define  WORK	4

#define  F_ESTADO	0x01
#define  F_VENTA	0x02
#define  F_TOTAL	0x04
#define  F_PRECIO	0x08

/* ***************************************************************************************************
 *                                            FUNCIONES                                              *
 *****************************************************************************************************/
// FUNCIONES DE MENOR NIVEL
long	get_crc_16( unsigned char *buf, int size );					// oK
int		EnviarID(uint8_t ID);											// oK
int		EnviarTrama( uint8_t ID, unsigned char *trama, int	n );		// oK
int		RecibirTrama( unsigned char *trama );						// oK		****
int		CerrarComunicacion(uint8_t ID, uint8_t consecutivo);				// oK
int		VerificaRecibido( unsigned char *trama, int n);

// FUNCIONES
int		getTotales(uint8_t ID, uint8_t manguera);							// oK
int		autorizar(uint8_t ID, uint8_t manguera, uint8_t *precioBDC);
int		desautorizar(uint8_t ID);										// oK
int		getVenta( uint8_t ID );										// oK
int		getEstado(uint8_t ID);											// oK

int		setPrecio( uint8_t ID, uint8_t surt, uint8_t lado, uint8_t manguera, unsigned int PPU );

/* ***************************************************************************************************
 *																									 *
 *****************************************************************************************************/
SoftwareSerial SerialIgem(4, 7);	// RX, TX

unsigned int PPUArray[3][2][4];		//  48 bytes. [Surtidor][lado][manguera]

/* ***************************************************************************************************
 *                                            VARIABLES                                              *
 *****************************************************************************************************/
uint8_t		IDs[3][2]= { {0x50, 0x51}, {0x52, 0x53}, {0x54, 0x55}};

uint8_t		F_ventaOk[3][2]		= { {0,0}, {0 ,0}, {0 ,0} };			// por CARA.
uint8_t		F_enviado[3][2]		= { {0,0}, {0 ,0}, {0 ,0} };			// por CARA.

uint8_t		F_globales[3][2]	= { {0,0}, {0 ,0}, {0 ,0} };			// por CARA.
uint8_t    	UltManguera[3][2]	= { {0,0}, {0 ,0}, {0 ,0} };			// por CARA.

int res;

/* ***************************************************************************************************
 *                              IMPLEMENTACION DE FUNCIONES MENOR NIVEL                              *
 *****************************************************************************************************/
// Primero se envia el uint8_t menos significativo
// despues el uint8_t mas significativo, se calcula con toda la trama, incluyendo el ID de la cara.
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
int		EnviarID( uint8_t ID )
{
	digitalWrite(TXE485,RS485Transmit);  digitalWrite(RXE485,RS485Transmit);
	
	//delay(1);
	SerialIgem.write( ID );
	SerialIgem.write(0x20);
	SerialIgem.write(0xfa);
	
	digitalWrite(TXE485,RS485Receive);  digitalWrite(RXE485,RS485Receive);
	
	Serial.print( F("TX ID: (") ); Serial.print( ID, HEX );	Serial.print( F(" 20 fa) > ") );	Serial.println( millis() );  // */
	return 0;
}

unsigned char ContRecepcion = 0x30;

// ----------------------------------------------------------------------------------------------------
uint8_t   ContEnvio[3][2] = { {0x30, 0x30}, {0x30, 0x30}, {0x30, 0x30} };		// las caras posibles.

int    EnviarTrama( uint8_t ID, unsigned char *trama, int n ) // 51 36 Trama crc 03 FA    // Ejp. | 50 | 33 | 65 1 2 | 62 C6 | 3 FA
{
	int i;
	unsigned int crc;
	
	int lado = (0x0f & ID)%2;
	int surt = (0x0f & ID)/2;
	
	// ----------------------------------------------------------------
	trama[0] = ID;
	trama[1] = ContEnvio[surt][lado];
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
	Serial.print(F("3 fa) > "));  Serial.println( millis() );
	
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
	unsigned long	tini_loop = millis();
  unsigned long  tini_prueba = millis();
  Serial.print(F("RX: ("));
  
	do {
		while(SerialIgem.available() > 0) {
			c = SerialIgem.read();
			Serial.print(0xff&(c), HEX);  Serial.print(F(" "));
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
  Serial.print(F("): "));  Serial.print(pos);  Serial.print(F(": Time: "));  Serial.print(millis()-tini_prueba);
  Serial.print(F(" < "));   Serial.println(millis());

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
	
	return pos;   // No se recibio nada.
}

// ----------------------------------------------------------------------------------------------------
int		EnviarTrama2( uint8_t ID, unsigned char *trama, int n )		// Con confirmacion.
{
	unsigned char trama2[10];
	
	int lado = (0x0f & ID)%2;
	int surt = (0x0f & ID)/2;

	for(int i=0; i<3; i++)
	{
		EnviarTrama(ID, trama, n);        // Envia solicitud de volumen.    51 35 | 1 1 0 | A2 50 | 3 FA
		res = RecibirTrama( trama2 );      // Validar trama recibida.        50 C5 FA
    delay(DELAYWAYNE);
		
		if(res>19)
		{
			Serial.println(F("PELIGRO* SE DESBORDO tramarecv... PELIGRO*****"));
			Serial.println(F("PELIGRO* SE DESBORDO tramarecv... PELIGRO*****"));
			Serial.println(F("PELIGRO* SE DESBORDO tramarecv... PELIGRO*****"));
		}
		if(res>=3)			// No se recibio nada.
		{
			ContEnvio[surt][lado]++;
			if(ContEnvio[surt][lado]>0x3f) ContEnvio[surt][lado] = 0x31;
			if( (trama2[1]&0xf0) == 0xc0 ) return 1;    // Recibido Ok.
			if( (trama2[1]&0xf0) == 0x50 ) ContEnvio[surt][lado] = 0x30;    // Reiniciar el contador de envio en tarjeta FULL y en wayne.
		}
		delay(DELAYWAYNE);    Serial.print(F("delay - "));    Serial.println(millis());		// En caso de error.
	}
	ContEnvio[surt][lado]++;
	if(ContEnvio[surt][lado]>0x3f) ContEnvio[surt][lado] = 0x31;
	
	return -1;
}

// ----------------------------------------------------------------------------------------------------
int    CerrarComunicacion(uint8_t ID, uint8_t consecutivo)
{
	/*  Surt  50 CE FA  */
	digitalWrite(TXE485,RS485Transmit);
	digitalWrite(RXE485,RS485Transmit);
	
	SerialIgem.write( ID );
	SerialIgem.write( 0xc0 + (0x0f&consecutivo) );
	SerialIgem.write( 0xfa );
	
	digitalWrite(TXE485, RS485Receive);
	digitalWrite(RXE485, RS485Receive);

  Serial.print(F("TX: ("));	Serial.print( ID, HEX );    Serial.print(" ");
	Serial.print( 0xc0 + (0x0f&consecutivo), HEX);    Serial.print(" ");
	Serial.println(F("fa)"));
	return 0;
}

// ----------------------------------------------------------------------------------------------------
// si llega 52 c3 fa (return 1) o 52 53 c3 (return -3)
unsigned long tmpnumeracion = 0;

uint8_t	finalVenta=0;
uint8_t  	validarDatos=0;		// Un ciclo de retraso para que valide datos.

int		VerificaRecibido( unsigned char *trama, int n)
{
	uint8_t	ret=0;
	uint8_t	F_locales = 0;
	
	// Funcion volumenes totales
	uint8_t total_mang = 0;				// cual fue la manguera que llega su total.
	
	// Funcion precios
	uint8_t precioBDC[5] = { 0, 0, 0, 0, 0 };
	unsigned int temp_precio = 0;
	uint8_t pre_mang = 0;
	uint8_t precio_mang_status = 0;
	uint8_t temp_estado = IDLE1;

	// Funcion precios
	unsigned long tempvolumen=0;
	unsigned long tempventa =0;
	
	//VERIFICACION DE CUAL CARA ES.
	uint8_t ID=0;
	unsigned int crc;
	int i=0;
	
	// --------------------------------------------------
	ID = trama[0];

	int lado = (0x0f & ID)%2;
	int surt = (0x0f & ID)/2;

	if(lado < 0) return 0;
	
	// --------------------------------------------------
	if(n<3)	return -1;						// No hay trazas de menos de tres uint8_ts.
	if(n==3)								// 53 c4 fa / 53 54 fa	(ok/error)
	{
		if(n ==0)   Serial.println(F("NO TRAZA..."));  // Mostros traza que llega.
		else
		{
//			for(int i=0; i<n; i++) { Serial.print(trama[i], HEX);  Serial.print(":"); }
//			Serial.println();
		}
		
		if(  trama[2] != 0xfa)							return -2;		// Error en el recibido.
		if( (trama[1]) == ((0x0f&ContEnvio[surt][lado])|0x50) )		return -3;		// Error en el recibido.
		if( (trama[1]) == ((0x0f&ContEnvio[surt][lado])|0xc0) )		return 1;		// Recibido Ok.
		return -4;
	}
	
	// ---------- VERIFICACION DE TRAZA RECIBIDA. -----------
	// Consecutivo
	//if(ContRecepcion != trama[1]) return -3;

	if(n >= 9)				// 51 35 1 1 5 da cb 3 fa
	{
		int pos_10 = -1;
		if (trama[n-4] == 0x10 && trama[n-3] == 0xFA) {
			pos_10 = n-4;
			n--;
		}
		if (trama[n-5] == 0x10 && trama[n-4] == 0xFA) {
			pos_10 = n-5;
			n--;
		}
		if(pos_10 != -1) {
			for (int i=pos_10; i<n; i++) {
				trama[i] = trama[i+1];
			}
		}

		crc = get_crc_16( trama, n-4 );
		for(int i=0; i<n; i++) {
			Serial.print(trama[i], HEX);  Serial.print(F(" "));
		} Serial.println();

		Serial.print(F("crc16 RX: "));  Serial.print(crc, HEX);
		Serial.print(F("- crc16 trama: "));  Serial.print((unsigned int)((trama[n-3]<<8) + trama[n-4]), HEX);

		if(crc != ((trama[n-3]<<8) + trama[n-4]))
		{	// ERROR CRC.
			Serial.println(F(" - ERROR ********* crc16"));
			return -6;
		} Serial.println();
    
		// ------------------------------------------------------
		CerrarComunicacion(ID, trama[1]);          						// Se cierra comunicacion.      50 CE FA
		
		int ciclos_n = 0;
		i = 2;
		while( (n-i)>4 )
		{
			Serial.print(F("----- n: "));	Serial.print(n);	Serial.print(F(" ,i: "));	Serial.println(i);
			if(ciclos_n++>10) {
				return -7;
    			Serial.print(F(" > "));  Serial.println( millis() );
			}
			
			// --------------------------------------------------
			if((trama[i]==0x10)&&(trama[i+1]==0xfa))
				i+=2;

			// --------------------------------------------------
			if((trama[i]==0x01)&&(trama[i+1]==0x01))
			{
				Serial.print(millis());   Serial.print(F("     **** Estado **** "));
				
				i+=2;
				temp_estado = trama[i++];
				
				F_locales |= F_ESTADO;			// pone a 1
			}
			
			// --------------------------------------------------
			if((trama[i]==0x02)&&(trama[i+1]==0x08))
			{							//			51 31 1 1 4 3 4 0 81 23 11 2 8 0 0 4 28 0 0 34 77 3  4 0 81 23 11 83 55 3 FA		Venta
				Serial.print(millis());   Serial.print(F("     **** Venta **** "));
				i+=2;
				
				// 4 uint8_ts 00 00 04 28 -> 0.428
				long  longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempvolumen  = 1000000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempvolumen += 10000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempvolumen += 100*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0X0f&trama[i++]);	tempvolumen += longTemp;
				
				//	4 uint8_ts 00 00 34 77 -> 3477
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempventa  = 1000000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempventa += 10000*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0x0f&trama[i++]);	tempventa += 100*longTemp;
				longTemp = 10*(0x0f&(trama[i]>>4)) + (0X0f&trama[i++]);	tempventa += longTemp;
				Serial.print(F("Vol: "));	Serial.print(tempvolumen);
				Serial.print(F(", Ven: "));	Serial.println(tempventa);
				
				F_locales |= F_VENTA;			// pone a 1
			}
			
			// --------------------------------------------------
			if((trama[i]==0x65)&&(trama[i+1]==0x10))
			{							//			51 39 65 10 1 0 0 42 66 33 0 0 42 66 33 0 0 0 0 0 A4 F0 3 FA
				Serial.print(millis());   Serial.println(F("     **** Totales **** "));
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
				Serial.print(millis());   Serial.print(F("     **** Precio **** "));
				i+=2;
				
				precioBDC[0] = trama[i];
				precioBDC[1] = trama[i+1];
				precioBDC[2] = trama[i+2];
				
				temp_precio  = 100000*(0x0f&(trama[i]>>4)) + 10000*(0x0f&trama[i++]); 	                      //	4 uint8_ts 00 81 23 -> 8123
				temp_precio +=   1000*(0x0f&(trama[i]>>4)) +   100*(0x0f&trama[i++]);
				temp_precio +=     10*(0x0f&(trama[i]>>4)) +       (0x0f&trama[i++]);
				
				precio_mang_status = 0x0f&(trama[i]>>4);			// Estado Manguera.
				pre_mang = 0x0f&trama[i++];					// Manguera (1, 2, 3).
				Serial.print(F("PPU: ")); Serial.print(temp_precio);
				Serial.print(F(", Mang: ")); Serial.println(pre_mang);
				
				//validarDatos=1;
				F_locales |= F_PRECIO;			// pone a 1
			}
			// Autorizacion.	Flag Autorizacion.		1 1 1 1 1 2
			/*	51 3D 1 1 1 1 1 2 1 1 2 3 4 0 81 23 11 B1 CE 3 FA:IG		Autorizacion
				51 3E 1 1 1 1 1 2 F7 4F 3 FA
				51 33 1 1 1 1 1 2 2B 8F 3 FA	*/
		}

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
			Serial.print(F("Manguera: "));		Serial.print(total_mang);
			
			Serial.print(F(" ,lado: "));		Serial.print(lado);    Serial.print(F(" + "));
			Serial.print(total_mang);			Serial.print(F(" | "));
			Serial.print(tmpnumeracion);		Serial.print(F(" - surtidor: "));				Serial.println(surt);
			
			// datos de totales.
			//if(mang_status[surt][lado]==IDLE1)
			if((mang_status[surt][lado]==IDLE1)&&(temp_precio != 0))
			{
				if((venta[surt][lado].Numeracion - tmpnumeracion)!=0) Serial.println(F("NO COINCIDE NUMERACION"));
				
				if(tmpnumeracion !=0) venta[surt][lado].Numeracion = tmpnumeracion;
				
				F_globales[surt][lado] |= F_TOTAL;
				F_locales &= ~F_TOTAL;
				
				ret = total_mang;
			}
		}
		
		// ---------------------------------------------------------------------------
		uint8_t uint8_ttemp=0;
		
		Serial.print(F("12 ID: "));   Serial.print(ID, HEX);  Serial.print(F(", lado: "));  Serial.println(lado);
		if(F_locales&F_PRECIO)
		{
			Serial.print(F("F_PRECIO| "));
			Serial.print(F("Manguera  : "));		Serial.print(pre_mang);     
			Serial.print(F(", Precio  : "));		Serial.print(temp_precio);
			Serial.print(F(", M_Before: "));		Serial.print(mang_status[surt][lado]);
			Serial.print(F(", M_Now   : "));		Serial.print(precio_mang_status);
			Serial.print(F(", surtidor: "));		Serial.print(surt);
			Serial.print(F(", lado    : "));		Serial.println(lado);
			
			if(temp_precio != 0)										// Si el precio en diferente de cero se actualiza.
			{
				UltManguera[surt][lado] = pre_mang;
				venta[surt][lado].PPU = temp_precio;
       
				F_globales[surt][lado] |= F_PRECIO;
			}
			//if(pre_mang != 0) 	venta[index].Manguera = pre_mang;
			
			// -------------------------
			if( (mang_status[surt][lado]==READY)&&(precio_mang_status==1) )		// INICIO DE LA VENTA	***
			{
				//desautorizar(ID);
				Serial.println(F("INICIA VENTA *******"));
				Serial.print(F("ID: "));  Serial.println(ID, HEX);
				
				//			AUTORIZACION
				autorizar(ID, pre_mang, precioBDC);
				
				F_ventaOk[surt][lado] = 0;
				F_globales[surt][lado] = 0;
			}
			
			/*
				Si esta manguera levantada y autorizacion 1 o 2, debe autorizar manguera nuevamente.
			*/
			// -------------------------
			if( (mang_status[surt][lado]== WORK)&&(precio_mang_status==0) )		// FINAL DE LA VENTA	***
			{
				desautorizar(ID);
				Serial.println(F("FIN VENTA ******* 1"));
				validarDatos=1;
				
				Serial.print(F("Manguera: "));  Serial.println(pre_mang);
				
				finalVenta=1;
				F_globales[surt][lado] = 0;
				F_ventaOk[surt][lado]=1;
				
				mang_status[surt][lado]=IDLE1;
				getVenta(ID);                   // Solicita VENTA
				//getTotales( ID, pre_mang );     // Solicita TOTALES.
				Serial.print(F(" > "));  Serial.println( millis() );
				return 0;
			}
			
			F_locales &= ~F_PRECIO;
			ret = pre_mang;
		}
		
		// ---------------------------------------------------------------------------
		if((F_locales&F_VENTA)&&(validarDatos == 1))
		{
			Serial.println(F("LLEGA VENTA 260081V"));

			EnviarID(ID);
			res = RecibirTrama( trama );
			delay(500);

			validarDatos=0;
			getVenta(ID);                   // Solicita VENTA
			getTotales( ID, pre_mang );     // Solicita TOTALES.
			Serial.print(F(" > "));  Serial.println( millis() );
			return 0;
		}
		
		if((F_locales&F_VENTA)&&(validarDatos == 0))
		{
			Serial.print(F("F_VENTA | "));
			Serial.print(F("Venta: "));			Serial.print(tempventa);
			Serial.print(F(" ,Volumen: "));		Serial.print(((float)tempvolumen)/1000);
			Serial.print(F(", surtidor:"));		Serial.print(surt);	Serial.print(F(", lado:"));		Serial.print(lado);			
			Serial.print(F(" - "));				Serial.println(millis());
			
			// @@@@@ aqui se debe revizar un posible error en los datos que se insertan en venta
			if(precio_mang_status==0)	// actualiza venta solo si la manguera esta colgada. hay que mirar lo de los totales...
			{
				venta[surt][lado].Venta = tempventa;
				venta[surt][lado].Volumen = ((double)tempvolumen)/1000;
				venta[surt][lado].manguera = pre_mang;

				if(finalVenta==1)
				{
					Serial.println(F("FIN VENTA ******* 2"));
					//desautorizar(ID);
					
					//if(venta[surt][lado].Venta==0)
					if(tempventa==0)
					{
						Serial.println(F("VENTA EN CEROS *******"));
						//desautorizar(ID);
					}
					
					finalVenta=0;
				}
			}
			
			F_globales[surt][lado] |= F_VENTA;
			F_locales &= ~F_VENTA;
		}
		
		uint8_t  puta = (uint8_t)F_VENTA|F_PRECIO|F_TOTAL;
		// ***************************************************************************
		if( (F_ventaOk[surt][lado]==1) && (F_globales[surt][lado] == puta) )
		{
			// datos de la venta
			Serial.print(F("VENTA NUEVA *************** "));
			Serial.print(F("Surtidor: "));			Serial.print(surt);
			Serial.print(F(", lado: "));			Serial.println(lado);
			Serial.print(F("Volumen: "));			Serial.println(venta[surt][lado].Volumen);
			Serial.print(F("Venta  : "));			Serial.println(venta[surt][lado].Venta);
			Serial.print(F("PPU    : "));			Serial.println(venta[surt][lado].PPU);
			Serial.print(F("manguera   : "));		Serial.println(venta[surt][lado].manguera);
			Serial.print(F("numeracion : "));		Serial.println(venta[surt][lado].Numeracion);

			long errorVenta = venta[surt][lado].Venta - (venta[surt][lado].Volumen*venta[surt][lado].PPU);
			Serial.print(F("error : "));	Serial.println(errorVenta);
			
			if(( errorVenta<-50)||(50<errorVenta))
			{
				mang_status[surt][lado]=1;
				
				F_ventaOk[surt][lado] = 0;			// No se envia esta venta y se permite releer en el proximo ciclo.
				F_globales[surt][lado] = 0;
	      Serial.print(F(" > "));  Serial.println( millis() );
				return -6;
			}
			
			F_enviado[surt][lado] = 0;
			F_ventaOk[surt][lado]= 0x10 + 2*surt + lado;    // 0-7 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			Serial.print(F("Volumen 2: ")); Serial.print(venta[surt][lado].Volumen);
      Serial.print(F(" > "));  Serial.println( millis() );
			return ret;
		}
    Serial.print(F(" > "));  Serial.println( millis() );
		return ret;
	}
}

/* ***************************************************************************************************
 *                              IMPLEMENTACION DE FUNCIONES MAYOR NIVEL                              *
 *****************************************************************************************************/
int    getEstado(uint8_t ID)         // Estado de cada cara del surtidor.
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
int    getTotales(uint8_t ID, uint8_t manguera)
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
int getVenta( uint8_t ID )   // Llenar una estructura con la informacion de la venta
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
int		autorizar(uint8_t ID, uint8_t manguera, uint8_t *precioBDC)			// Por ahora solo autoriza a cualquier monto.
{
	Serial.println(F("Autorizar Venta *******"));
	Serial.print(F("ID   : "));	Serial.print(ID);	Serial.print(F(", mang : "));	Serial.println(manguera);
 
  uint8_t surtidor = i2cAutoriza.surtidor;
  uint8_t lado = i2cAutoriza.lado;
	
	if((funAuth[surtidor][lado].funcion != AUTORIZAR)||(millis() > funAuth[surtidor][lado].time))
	{
    funAuth[surtidor][lado].funcion =0;
		Serial.println(F("*** No hay AUTORIZACION ***"));
		return -1;
	}

	int		mang = i2cAutoriza.mang;
	uint8_t	modo = i2cAutoriza.modo;
	double	cantidad = i2cAutoriza.cantidad;

	// probar si la manguera es la misma, sino, retorna.
	Serial.println(F("--- i2cAutoriza ---"));
	Serial.print(F("modo    : "));  Serial.println(modo);
	Serial.print(F("lado    : "));  Serial.println(lado);
	Serial.print(F("mang    : "));  Serial.println(mang);
	Serial.print(F("cantidad: "));  Serial.println(cantidad);
  
	if((manguera-1) != mang)
	{
    funAuth[surtidor][lado].funcion =0;
		Serial.println(F("*** Manguera DIFERENTE a la seleccionada ***"));
		return -2;
	}

	Serial.println(F("--- verifica lado ---"));
	Serial.print(F("ID1    : "));  Serial.println(IDs[surtidor][lado], HEX);
	Serial.print(F("ID2    : "));  Serial.println(ID, HEX);
	Serial.println();
	
	if( IDs[surtidor][lado] != ID )
	{
    funAuth[surtidor][lado].funcion =0;
    Serial.println(F("*** Numero de lado o cara no corresponde a la autorizada ***"));
		return -1;
	}
  
	int res = 0;
	unsigned char trama[20];
	unsigned int crc;
	
	trama[2] = 0x02;
	trama[3] = 0x01;
	trama[4] = manguera;
	EnviarTrama2(ID, trama, 3);        // 51 32 2 1 1 92 E4 3 fa
	
	trama[2] = 0x01;
	trama[3] = 0x01;
	trama[4] = 0x05;
	EnviarTrama2(ID, trama, 3);        // 51 33 1 1 5 62 DB 3 fa
	
	trama[2] = 0x02;
	trama[3] = 0x01;
	trama[4] = manguera;
	EnviarTrama2(ID, trama, 3);        // 51 34 2 1 1 92 6C 3 fa
	
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
	
	// calcular el valor.
	long ventaBin = i2cAutoriza.cantidad;
	uint8_t ventaBDC[8];
	
	ventaBDC[0] = 0x0f&((ventaBin%100000000)/10000000);
	ventaBDC[1] =  0x0f&((ventaBin%10000000)/1000000);
	ventaBDC[2] =   0x0f&((ventaBin%1000000)/100000);
	ventaBDC[3] =     0x0f&((ventaBin%100000)/10000);
	ventaBDC[4] =       0x0f&((ventaBin%10000)/1000);
	ventaBDC[5] =         0x0f&((ventaBin%1000)/100);
	ventaBDC[6] =           0x0f&((ventaBin%100)/10);
	ventaBDC[7] =             0x0f&(ventaBin%10);

	trama[4] = (0xf0&(ventaBDC[0]<<4)) | (0x0f&ventaBDC[1]); // MSB
	trama[5] = (0xf0&(ventaBDC[2]<<4)) | (0x0f&ventaBDC[3]); // MSB
	trama[6] = (0xf0&(ventaBDC[4]<<4)) | (0x0f&ventaBDC[5]);
	trama[7] = (0xf0&(ventaBDC[6]<<4)) | (0x0f&ventaBDC[7]); // LSB
	EnviarTrama2(ID, trama, 6);        // 51 35 3 4 99 99 99 99 4 89 3 fa

	trama[2] = 0x01;
	trama[3] = 0x01;
	trama[4] = 0x06;
	EnviarTrama2(ID, trama, 3);        // 51 36 1 1 6 22 16 3 fa
  
  mang_status[surtidor][lado]= WORK;
  funAuth[surtidor][lado].funcion=0;
	return 0;
}

// ----------------------------------------------------------------------------------------------------
int		desautorizar(uint8_t ID)
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
uint16_t PPUs[3][2][4];		// 24

int		setPrecio( uint8_t ID, uint8_t surt, uint8_t lado, uint8_t manguera, unsigned int PPU_ )		// mierda
{
	Serial.print(F("ID: "));  Serial.print(ID, HEX);	Serial.print(F(", Mang: "));  Serial.print(manguera);	Serial.print(F(", precio: "));  Serial.println(PPU_);
	// 51 3F 5 3 0 81 23 A8 7 3 FA						Una manguera
	// 52 3C 5 9 0 12 34  0 56 78 0 26 53 A9 46 3 FA	Tres mangueras
	unsigned char trama[21];
	for(int i=0; i<sizeof(trama); i++) trama[i]=0;

	trama[2] = 0x05;
	if(manguera==0) trama[3] = 0x03   ;// si manguera es 1.
	if(manguera==1) trama[3] = 0x06   ;// si manguera es 2.
	if(manguera==2) trama[3] = 0x09   ;// si manguera es 3.
	
	PPUs[surt][lado][manguera] = PPU_;  // almacenar el precio que se quiere cambiar.
	
	Serial.print(F("surt: "));    		Serial.print( surt );		Serial.print(F(", lado: "));    Serial.print( lado );
	Serial.print(F(", manguera: "));	Serial.print( manguera );	Serial.print(F(", PPUs: "));    Serial.println(PPU_);
	
	for(int i=1; i<=manguera+1; i++) {
		uint8_t		precioBDC[6];
		uint16_t	PPU = PPUs[surt][lado][i-1];

		precioBDC[0] = 0x0f&( (PPU%1000000) /100000);
		precioBDC[1] =   0x0f&( (PPU%100000) /10000);
		precioBDC[2] =     0x0f&( (PPU%10000) /1000);
		precioBDC[3] =       0x0f&( (PPU%1000) /100);
		precioBDC[4] =         0x0f&( (PPU%100) /10);
		precioBDC[5] =           0x0f&( PPU%10 );

		for(uint8_t k=0; k<6; k++) {
			Serial.print(precioBDC[k], HEX);      Serial.print(F(" "));
		}      Serial.println(PPU);

		trama[3*i+1] = (0xf0&(precioBDC[0]<<4)) | (0x0f&precioBDC[1]); // MSB
		trama[3*i+2] = (0xf0&(precioBDC[2]<<4)) | (0x0f&precioBDC[3]);
		trama[3*i+3] = (0xf0&(precioBDC[4]<<4)) | (0x0f&precioBDC[5]); // LSB
	}
  
	if(manguera==0) EnviarTrama2(ID, trama, 5);     // si manguera es 1.
	if(manguera==1) EnviarTrama2(ID, trama, 8);     // si manguera es 2.
	if(manguera==2) EnviarTrama2(ID, trama, 11);    // si manguera es 3.  */
	
	trama[2] = 0x02;
	trama[3] = 0x01;
	trama[4] = manguera+1;
	EnviarTrama2(ID, trama, 3);        // 51 32 2 1 1 92 E4 3 fa
  
	trama[2] = 0x01;
	trama[3] = 0x01;
	trama[4] = 0x05;
	EnviarTrama2(ID, trama, 3);        // 51 36 1 1 8 22 16 3 fa
  
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
