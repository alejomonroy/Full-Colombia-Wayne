#define Num_surtidores 1   //Si se monta solo un promini se pone 1
                           //si se montan ambos, poner 2. 

void RecibeRequisicionI2C( int howMany );

void  delayx(long tiempo)
{
	long tinix;

	tinix= millis();
	while((millis()-tinix)<tiempo);
}

void  bzero(char *str, int tam)
{
	for(int i=0; i<tam; i++) str[i]=0;
}

// -----------------------------------------------------------------
char char2int(char input)
{
	if( ('0'<=input) && (input<='9') )  return (input-'0');
	if( ('a'<=input) && (input<='f') )  return (input-'a'+10);
	if( ('A'<=input) && (input<='F') )  return (input-'A'+10);
	return 0;
}

/* ******************************************************
 *               iButton           *
 ********************************************************/
typedef struct
{
	char  id[18];
	byte  lado;
	int   Check;
}Ibutton;

/* ***************************************************************************************************
 *                                    Estructura de Configuracion                                    *
 *****************************************************************************************************/
typedef struct
{
	unsigned long Numeracion1[3];		// Cara 1.
	unsigned long Numeracion2[3];		// Cara 2.
}	Numeracion;

// ----------------------------------------------------------------------------------------------------
void LoopProtocolo_wayne()		// Codigo para comunicacion con surtidor Marca WAYNE.
{
	int manguera=0;
	
	for( int i=0; i<4; i++ )	// 
	{
		unsigned char trama[100];

		// -------------------------------------------------
		if((0x01&ContLoop)==0)
			Verificar_iButton(i);
		
		// ------------------- ENVIAR ID -------------------
		getEstado(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);
		delay(DELAYWAYNE);
		
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);
		delay(DELAYWAYNE);
		
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);
		delay(DELAYWAYNE);
		
		EnviarID(IDs[i]);
		res = RecibirTrama( trama );
		if(res >= 3)   manguera = VerificaRecibido( trama, res);
		delay(DELAYWAYNE);
	}

	print_infoVenta(0);
	print_infoVenta(1);
	print_infoVenta(2);
	print_infoVenta(3);
	print_infoVenta(4);
	print_infoVenta(5);		// las 6 mangueras.
	delayx(50);
// */	
	if((0x03&ContLoop)==0)	// Envia.
	{
		int estadoPin;
		int i=0;
    
		strcpy( strI2C, F("innpe:1:venta:") );		// No importa que cara si es A, B o C.
		
		// -------------------------
		// ENVIAR DATOS AL ARDUINO PRINCIPAL.
		tmpstr1 = (char*)(&venta);
		for(int i=0; i<sizeof(venta); i++)
		{
			chari2c = tmpstr1[i];
			
			if( chari2c<16 )  sprintf( strI2C, "%s0%x", strI2C, chari2c );
			else        sprintf( strI2C,  "%s%x", strI2C, chari2c );
		}
		Serial.println(strI2C);		// estructura que contiene la informacion a enviar. 182 bytes (2020-02-02).
		
		int    intTemp;
		i=0;
		intTemp = strlen(strI2C);
		while( 32*i< intTemp )
		{
			char  bufI2C[55];
			bzero(bufI2C, sizeof(bufI2C));
      
			strncpy(bufI2C, &strI2C[i*32], 32);
			Serial.println(bufI2C);
			
			Wire.beginTransmission(ARD_MEGA2560);
			Wire.write(bufI2C);
			Wire.endTransmission();
			
			delayx(15);
			i++;
		}
		
		Wire.beginTransmission(ARD_MEGA2560);
		Wire.write("END");
		Wire.endTransmission();
		
		delayx(100); // */
		
		// --------------------------------------------------
		// --------------------------------------------------
		// --------------------------------------------------
		if(Num_surtidores == 2)
		{
			delayx(100);
			NumeracionS2   totalS2;
			VentaS2   		ventaS2;
			int varI = 0;
			char  cI2C;
			char  strTemp[20];
			
			char *tmpstr = (char*)(&ventaS2);
			
			// Solicitud				SI ENTRA AQUI, SIEMPRE SOY EL 1.
			strcpy_P( strTemp, (PGM_P)F("innpe:1:venta1:0"));   Serial.println(strTemp);
			Wire.beginTransmission(ARD_PROTOCOLO2);
			Wire.write(strTemp);
			Wire.endTransmission();
			delayx(5);
			
//			estadoPin= digitalRead(A4);   Serial.print(F("SDA: "));   Serial.println(estadoPin);
//			estadoPin= digitalRead(A5);   Serial.print(F("SCL: "));   Serial.println(estadoPin);
			
			// Requisicion y recepcion
			Wire.requestFrom(ARD_PROTOCOLO2, 2*sizeof(ventaS2));    // request 6 bytes from slave device #8
			varI = 0;
			while(Wire.available() > 0)
			{
				cI2C = Wire.read();
				
				if(varI<199) strI2C[varI++] = cI2C;
			}	strI2C[varI]=0;
			Serial.print(F("strI2C: "));    Serial.println(strI2C);
			
			// Monta en estructura
			tmpstr = (char*)(&ventaS2);
			
			for(i=0; i<sizeof(ventaS2); i++)
			{
				char chari2c = (char2int( strI2C[2*i] )<<4) | char2int( strI2C[2*i+1] );
				tmpstr[i] = chari2c;
			}
			
			i = 3*ventaS2.manguera;
			venta2[0].Volumen	= ventaS2.Volumen;
			venta2[0].Venta		= ventaS2.Venta;
			venta2[0].PPU		= ventaS2.PPU;
			
			Serial.print(F("M:       "));    Serial.println(ventaS2.manguera);
			Serial.print(F("i:       "));    Serial.println(i);
			Serial.print(F("Volumen: "));    Serial.println(ventaS2.Volumen);
			Serial.print(F("Venta:   "));    Serial.println(ventaS2.Venta);
			Serial.print(F("PPU:     "));    Serial.println(ventaS2.PPU);
			
			// --------------------------------------------------
			//estadoPin= digitalRead(A4);   Serial.print(F("SDA: "));   Serial.println(estadoPin);
			//estadoPin= digitalRead(A5);   Serial.print(F("SCL: "));   Serial.println(estadoPin);
			// Solicitud
			strcpy_P( strTemp, (PGM_P)F("innpe:1:total1:0"));		Serial.println(strTemp);
			Wire.beginTransmission(ARD_PROTOCOLO2);
			Wire.write(strTemp);
			Wire.endTransmission();
			delayx(5);
			
			//estadoPin= digitalRead(A4);   Serial.print(F("SDA: "));   Serial.println(estadoPin);
			//estadoPin= digitalRead(A5);   Serial.print(F("SCL: "));   Serial.println(estadoPin);
			// Requisicion y recepcion
			Wire.requestFrom(ARD_PROTOCOLO2, 2*sizeof(totalS2));    // request 6 bytes from slave device #8
			varI = 0;
			while (Wire.available() > 0)
			{
				cI2C = Wire.read();
				
				if(varI<199) strI2C[varI++] = cI2C;
			}	strI2C[varI]=0;
			Serial.print(F("strI2C: "));    Serial.println(strI2C);
			
			// Monta en estructura
			tmpstr = (char*)(&totalS2);
			for(i=0; i<sizeof(totalS2); i++)
			{
				char chari2c = (char2int( strI2C[2*i] )<<4) | char2int( strI2C[2*i+1] );
				tmpstr[i] = chari2c;
			}
			
			i = 3*totalS2.manguera;   // 0 => 2 - 1 => 5
			venta2[0].Numeracion   = totalS2.Numeracion;		// mierda puta reputa
			
			Serial.print(F("M  : "));    Serial.println(totalS2.manguera);
			Serial.print(F("i  : "));    Serial.println(i);
			Serial.print(F("Num: "));    Serial.println(totalS2.Numeracion);
			
			// --------------------------------------------------
			//estadoPin= digitalRead(A4);   Serial.print(F("SDA: "));   Serial.println(estadoPin);
			//estadoPin= digitalRead(A5);   Serial.print(F("SCL: "));   Serial.println(estadoPin);
			
			// Solicitud
			strcpy_P( strTemp, (PGM_P)F("innpe:1:venta2:0"));		Serial.println(strTemp);
			Wire.beginTransmission(ARD_PROTOCOLO2);
			Wire.write(strTemp);
			Wire.endTransmission();
			delayx(5);
			
			//estadoPin= digitalRead(A4);   Serial.print(F("SDA: "));   Serial.println(estadoPin);
			//estadoPin= digitalRead(A5);   Serial.print(F("SCL: "));   Serial.println(estadoPin);
			// Requisicion y recepcion
			Wire.requestFrom(ARD_PROTOCOLO2, 2*sizeof(ventaS2));    // request 6 bytes from slave device #8
			varI = 0;
			while (Wire.available() > 0)
			{
				cI2C = Wire.read();
				
				if(varI<199) strI2C[varI++] = cI2C;
			}	strI2C[varI]=0;
			Serial.print(F("strI2C: "));    Serial.println(strI2C);
			
			// Monta en estructura
			tmpstr = (char*)(&ventaS2);
			for(i=0; i<sizeof(ventaS2); i++)
			{
				char chari2c = (char2int( strI2C[2*i] )<<4) | char2int( strI2C[2*i+1] );
				tmpstr[i] = chari2c;
			}
			
			i = 3*ventaS2.manguera;   // 0 => 2 - 1 => 5
			venta2[3].Volumen  = ventaS2.Volumen;
			venta2[3].Venta    = ventaS2.Venta;
			venta2[3].PPU    = ventaS2.PPU;
			
			Serial.print(F("M: "));    Serial.println(ventaS2.manguera);
			Serial.print(F("i: "));    Serial.println(i);
			Serial.print(F("Volumen: "));    Serial.println(ventaS2.Volumen);
			Serial.print(F("Venta  : "));    Serial.println(ventaS2.Venta);
			Serial.print(F("PPU    : "));    Serial.println(ventaS2.PPU);
			
			// --------------------------------------------------
			//estadoPin= digitalRead(A4);   Serial.print(F("SDA: "));   Serial.println(estadoPin);
			//estadoPin= digitalRead(A5);   Serial.print(F("SCL: "));   Serial.println(estadoPin);
			// Solicitud
			strcpy_P( strTemp, (PGM_P)F("innpe:1:total2:0"));		Serial.println(strTemp);
			Wire.beginTransmission(ARD_PROTOCOLO2);
			Wire.write(strTemp);
			Wire.endTransmission();
			delayx(5);
			
			//estadoPin= digitalRead(A4);   Serial.print(F("SDA: "));   Serial.println(estadoPin);
			//estadoPin= digitalRead(A5);   Serial.print(F("SCL: "));   Serial.println(estadoPin);
			// Requisicion y recepcion
			Wire.requestFrom(ARD_PROTOCOLO2, 2*sizeof(totalS2));    // request 6 bytes from slave device #8
			varI = 0;
			while (Wire.available() > 0)
			{
				cI2C = Wire.read();
			
				if(varI<199) strI2C[varI++] = cI2C;
			}	strI2C[varI]=0;
			Serial.print(F("strI2C: "));    Serial.println(strI2C);
			
			// Monta en estructura
			tmpstr = (char*)(&totalS2);
			for(i=0; i<sizeof(totalS2); i++)
			{
				char chari2c = (char2int( strI2C[2*i] )<<4) | char2int( strI2C[2*i+1] );
				tmpstr[i] = chari2c;
			}
			
			i = 3*totalS2.manguera + 2;   // 0 => 2 - 1 => 5      @@@@@ mierda????
			venta2[3].Numeracion   = totalS2.Numeracion;
			
			Serial.print(F("M  : "));    Serial.println(totalS2.manguera);
			Serial.print(F("i  : "));    Serial.println(i);
			Serial.print(F("Num: "));    Serial.println(totalS2.Numeracion);
			
			strcpy( strI2C, F("innpe:2:venta:") );		// Siempre va a llegar como SURTIDOR 1
			// -------------------------
			// ENVIAR DATOS AL ARDUINO PRINCIPAL.
			tmpstr1 = (char*)(&venta2);
			for(int i=0; i<sizeof(venta2); i++)
			{
				chari2c = tmpstr1[i];
				
				if( chari2c<16 )  sprintf( strI2C, "%s0%x", strI2C, chari2c );
				else        sprintf( strI2C,  "%s%x", strI2C, chari2c );
			}
			Serial.println(strI2C);		// estructura que contiene la informacion a enviar.
			
			int    intTemp;
			int i=0;
			intTemp = strlen(strI2C);
			while( 32*i< intTemp )
			{
				char  bufI2C[55];
				bzero(bufI2C, sizeof(bufI2C));
				
				strncpy(bufI2C, &strI2C[i*32], 32);
				Serial.println(bufI2C);
				
				Wire.beginTransmission(ARD_MEGA2560);
				Wire.write(bufI2C);
				Wire.endTransmission();
				
				delayx(15);
				i++;
			}
			
			Wire.beginTransmission(ARD_MEGA2560);
			Wire.write("END");
			Wire.endTransmission();
			
			delayx(10);   // */
		}
	}

}

// ----------------------------------------------------------------------------------------------------
void Verificar_iButton(int cara)
{
  if( trazaI2C == 0 )             // 
  {
    byte      addr[8];      // Almacena la informacion que se lee desde el ibutton.
    char  Cod_iButton[20];
    
    //Serial.print( F("trazaI2C: ") );  Serial.println(trazaI2C);
    //---------------------------------------------------------------
    byte  No_ds = 0;
    
    if( (cara==0)||(cara==2) )  getKeyCode(ds1, addr);
    if( (cara==1)||(cara==3) )  getKeyCode(ds2, addr);
    
    if(keyStatus==F("ok"))       // Se recibio dato desde iButton.
    {
      // PASAR DE HEXAGESIMAL A ASCII.
		Cod_iButton[0] = 0;
		for(int i = 8; i >0; i--)   // ED:00:00:15:BE:65:93:01    MOSTRAR IBUTTON.
		{
			Serial.print( F(":"));    Serial.print(addr[i-1], HEX);
        
			if( addr[i-1]<16 )  sprintf( Cod_iButton, "%s0%x", Cod_iButton, addr[i-1] );
			else        sprintf( Cod_iButton,  "%s%x", Cod_iButton, addr[i-1] );
		}
		Serial.print( F("Enviar: ") );    Serial.println(Cod_iButton);              // -----------------------------------------
      
		// ------------------------------------
		char  strIbutton1[20];
		strcpy(strIbutton1, F("innpe:1:iButton:"));
      
		Wire.beginTransmission(ARD_MEGA2560);
		if( (cara==0)||(cara==2) )  Wire.write( strIbutton1 );    // 18
		if( (cara==1)||(cara==3) )  Wire.write( strIbutton1 );    // 18
		Wire.endTransmission();
      
		if( (cara==0)||(cara==2) )  Serial.print( F("innpe:1:iButton:") );    // 18
		if( (cara==1)||(cara==3) )  Serial.print( F("innpe:1:iButton:") );    // 18
      
      // ------------------------------------
		Ibutton ibutton;
		strcpy(ibutton.id, Cod_iButton);
		ibutton.lado = cara+1;
      
		int sumatoria=0;
		for(int i=0; i<18; i++) sumatoria = sumatoria+ 0xff&((int)ibutton.id);
		sumatoria = sumatoria+ (int)ibutton.lado;
		ibutton.Check = sumatoria;

		Serial.print(ibutton.Check);
		Serial.print(" - ");
		Serial.println(sumatoria);
      
		int i;
		strI2C[0]=0;
		tmpstr1 = (char*)(&ibutton);
		for(i=0; i<sizeof(ibutton); i++)
		{
			chari2c = tmpstr1[i];
        
			if( chari2c<16 )  sprintf( strI2C, "%s0%x", strI2C, chari2c );
			else        sprintf( strI2C,  "%s%x", strI2C, chari2c );
		}
		Serial.println(strI2C);   // estructura que contiene la informacion a enviar.
      
		i=0;
		int    intTemp;
		intTemp = strlen(strI2C);
		while( 32*i< intTemp )
		{
			char  bufI2C[55];
			bzero(bufI2C, sizeof(bufI2C));
        
			strncpy(bufI2C, &strI2C[i*32], 32);
			Serial.println(bufI2C);
        
			Wire.beginTransmission(ARD_MEGA2560);
			Wire.write(bufI2C);
			Wire.endTransmission();
        
			delay(7);
        
			i++;
		}
      
      // ------------------------------------
		Wire.beginTransmission(ARD_MEGA2560);
		Wire.write("END");            // 3
		Wire.endTransmission();
		Serial.println( F("END") );
      
    }
  }
//  Serial.print( F("iButton: ") );   Serial.println(millis());

}

// ----------------------------------------------------------------------------------------------------
void LoopI2C_Comunicacion()
{
	//--------------------------------------------------------------- */
	//							AUTORIZAR							  */
	//--------------------------------------------------------------- */
	if( trazaI2C == 3 )		// recibir Autorizar.
	{
		Serial.print( F("Recibido de principal: ") );		Serial.println( trazaI2C );
		
		if(Autorizacion == 1)
		{
/*			if( strcmp(DatosI2C, "0")==0 )	C1_Placa[0]=0;
			else							strcpy( C1_Placa, DatosI2C);				// Placa Cara 1.
			
			mang_status[0] = 0;
			Serial.print( F("AUTORIZAR MANGUERAS **A** ") );   Serial.println( C1_Placa );  */
		}
		
		if(Autorizacion == 2)
		{
/*			if( strcmp(DatosI2C, "0")==0 )	C2_Placa[0]=0;
			else							strcpy( C2_Placa, DatosI2C);				// Placa Cara 2.
			
			mang_status[1] = 0;
			Serial.print( F("AUTORIZAR MANGUERAS **B** ") );   Serial.println( C2_Placa );  */
		}
		
//		*****
		iB_Tini = millis();
		Serial.print( F("millis 2: ") );    Serial.println( millis() );
		Serial.print( F("iB_Tini 2: ") );    Serial.println( iB_Tini );
		trazaI2C = 0;
	}	// FIN AUTORIZAR.	*/
	
	//--------------------------------------------------------------- */
	//							ENVIAR VENTA						  */
	//--------------------------------------------------------------- */
	if( trazaI2C == 1 )   // Solicitud de enviar venta. PRINCIPAL: MEGA2560
	{
		// Codigo en caso de requerir una venta.
		
		trazaI2C = 0;
	} // FIN ENVIAR VENTA.
	
	//--------------------------------------------------------------- */
	//							VOLUMENES							  */
	//--------------------------------------------------------------- */
	// ------------------------------------ numeracion ------------------------------------
	if( trazaI2C == 2 )   // Solicitud de enviar venta. PRINCIPAL: MEGA2560
	{
		Numeracion  numeracion;
		
		Serial.print(F("(2). DatosI2C: "));    Serial.print(DatosI2C);    Serial.print(F(" - "));    Serial.println(millis());
		
		for(int i=0; i<4; i++)							// BORRAR NUMERACIONES.
		{
			numeracion.Numeracion1[i]=0;
			numeracion.Numeracion2[i]=0;
		}
		
		Serial.println(F("         ************************************"));
		Serial.println(F("                       Volumenes             "));
		Serial.println(F("         ************************************"));
		
		for(int i=1; i<=2; i++)				// Cada manguera. (1, 2, 3)
		{
			Serial.print(F("Manguera: "));	Serial.println(i);
			
			for(int j=0; j<4; j++)			// Cada cara.
			{
				Serial.print(F("  Cara:"));		Serial.println(IDs[j], HEX);
				
				// i -> Manguera(1 , 2), j -> Cara(50, 51)
				getTotales(IDs[j], i);		// primer argumento CARA, segundo argumento Manguera.
				tmpnumeracion =0;
				
//				for(int k=0; k<4; k++)
//				{
					unsigned char trama[100];
          
					EnviarID(IDs[j]);       // ENVIAR ID.
					res = RecibirTrama( trama );
					if(res >= 3)   VerificaRecibido( trama, res);

//					if(tmpnumeracion != 0) break;
//				}
          // si tmpnumeracion==0 verificar el volumen de ultima venta y ver que ocurre.
				if(tmpnumeracion == 0) continue;
				
				Serial.println();
				Serial.print(F("i-1: "));	Serial.print(i-1);	Serial.print(F(", j: "));	Serial.println(j);
				Serial.print(F("numeracion: "));	Serial.println(tmpnumeracion);
				
				if(j==0)          numeracion.Numeracion1[i-1] = tmpnumeracion;    // volumen1_H - CARA A
				if(j==1)          numeracion.Numeracion2[i-1] = tmpnumeracion;    // volumen2_H - CARA B
				if(j==2)          numeracion.Numeracion1[2] = tmpnumeracion;    // volumen1_H - CARA A
				if(j==3)          numeracion.Numeracion2[2] = tmpnumeracion;    // volumen2_H - CARA B

			}
		}
		
		Serial.println(F("VOLUMENES..."));
		for(int i=0; i<3; i++)
		{
			Serial.print(numeracion.Numeracion1[i]);
			Serial.print(F(" - "));
			Serial.print(numeracion.Numeracion2[i]);	Serial.println(F("|"));
		}
		
		// Enviar al PRINCIPAL los volumenes para el turno CIERRE/APERTURA.
		strcpy( strI2C, F("innpe:1:numeracion:") );		// "innpe:1:numeracion:"
		
		// -------------------------
		tmpstr1 = (char*)(&numeracion);
		for(int i=0; i<sizeof(numeracion); i++)
		{
			chari2c = tmpstr1[i];
			
			if( chari2c<16 )  sprintf( strI2C, "%s0%x", strI2C, chari2c );
			else        sprintf( strI2C,  "%s%x", strI2C, chari2c );
		}
		Serial.println(strI2C);		// estructura que contiene la informacion a enviar.
		
		int i=0;
		int intTemp = strlen(strI2C);
		while( 32*i< intTemp )
		{
			char  bufI2C[55];
			bzero(bufI2C, sizeof(bufI2C));
      
			strncpy(bufI2C, &strI2C[i*32], 32);
			Serial.println(bufI2C);
			
			Wire.beginTransmission(ARD_MEGA2560);
			Wire.write(bufI2C);
			Wire.endTransmission();
			
			i++;
		}
		
		// --------------------------------------------------
		Serial.println( F("Enviada traza I2C") );
		
		Wire.beginTransmission(ARD_MEGA2560);
		Wire.write("END");
		Wire.endTransmission();
		
		trazaI2C = 0;
	} // FIN ENVIAR VENTA.
	
	//--------------------------------------------------------------- */
	//						ESTADO DE MANGUERAS						  */
	//--------------------------------------------------------------- */
	if( trazaI2C == 5 )			// Estado mangueras. ( ladoA, ladoB, ambos ).
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
		
		Wire.beginTransmission(ARD_MEGA2560);
		Wire.write( strTemp );
		Wire.endTransmission();
		
		delayx(7);		// Este delay se debe a que se eliminaron los bloqueos en la libreria twi.
		
		Wire.beginTransmission(ARD_MEGA2560);
		Wire.write( "END" );
		Wire.endTransmission();
		
		Serial.println(F("FIN ESTADO MANGUERAS..."));
		trazaI2C = 0;
	}
	
	if( trazaI2C == 6 )
	{
		trazaI2C = 0;
	} // */
 
	if( PPUI2C == 7 )   // Actualizar precio de mangueras.
	{
		Serial.println(F("CAMBIANDO PRECIOS..."));
		for(int i=0; i<6; i++)
    {
		if(PPUArray[i]!=0)
		{
			Serial.print(F("i  : "));        Serial.println(i);
			Serial.print(F("PPU:"));         Serial.println(PPUArray[i]);
			if(i==0) setPrecio(IDs[0], 1, PPUArray[i]);
			if(i==1) setPrecio(IDs[0], 2, PPUArray[i]);
			if(i==2) setPrecio(IDs[2], 1, PPUArray[i]);
       
			if(i==3) setPrecio(IDs[1], 1, PPUArray[i]);
			if(i==4) setPrecio(IDs[1], 2, PPUArray[i]);
			if(i==5) setPrecio(IDs[3], 1, PPUArray[i]);
        
			PPUArray[i]=0;
		}
    }
    
    PPUI2C = 0;
    trazaI2C = 0;
  } // 
	
}

// ---------------------------------------------------------------
char *itostr( char *varstr, byte vardato )
{
	if(vardato<10)  sprintf( varstr, "0%d", vardato );
	else      sprintf( varstr, "%d", vardato );
	return varstr;
}
