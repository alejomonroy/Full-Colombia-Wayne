void LoopI2C_Comunicacion();

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
void print_infoVenta( uint8_t surtidor, uint8_t lado )		// muestra la ultima venta del surtidor.
{
  char  strVolumen[15];
	
	Serial.print( F("----- surtidor: ") );  Serial.print(surtidor);
	Serial.print( F(", lado: ") );  Serial.println(lado);
	
	dtostrf( venta[surtidor][lado].Volumen, 4, 3, strVolumen);
	Serial.print( F("Volumen: ") );     Serial.println( strVolumen );
	Serial.print( F("Venta: ") );     Serial.println( venta[surtidor][lado].Venta );
	Serial.print( F("PPU: ") );       Serial.println( venta[surtidor][lado].PPU );
	Serial.print( F("Numeracion: ") );      Serial.println( venta[surtidor][lado].Numeracion );
}

// -----------------------------------------------------------------
void  bzero(char *str, int tam){	memset(str, 0, tam);	}

// -----------------------------------------------------------------
char char2int(char input)
{
	if( ('0'<=input) && (input<='9') )  return (input-'0');
	if( ('a'<=input) && (input<='f') )  return (input-'a'+10);
	if( ('A'<=input) && (input<='F') )  return (input-'A'+10);
	return 0;
}

// ----------------------------------------------------------------------------------------------------
void LoopProtocolo_wayne()		// Codigo para comunicacion con surtidor Marca WAYNE.
{
	for( int surt=0; surt<Conf.Num_Surt; surt++ )	// surtidores
	{
    while(SYNC) LoopI2C_Comunicacion();
    delay(40);    SYNC = 1;
    
		for( int lado=0; lado<2; lado++ )	// surtidores
		{
			unsigned char trama[100];

			// -------------------------------------------------
			//	Verificar_iButton(lado);
			
			// ------------------- ENVIAR ID -------------------
      Serial.println(F("_____"));
			getEstado(IDs[surt][lado]);
      Serial.println(F("_____"));
			
			EnviarID(IDs[surt][lado]);
			res = RecibirTrama( trama );
			if(res >= 3)   VerificaRecibido( trama, res);
			
			EnviarID(IDs[surt][lado]);
			res = RecibirTrama( trama );
			if(res >= 3)   VerificaRecibido( trama, res);
			
			EnviarID(IDs[surt][lado]);
			res = RecibirTrama( trama );
			if(res >= 3)   VerificaRecibido( trama, res);
		}
	}

  for(int surt=0; surt<3; surt++) {
    for(int lado=0; lado<2; lado++) {
      print_infoVenta(surt, lado);
    }
  }
  
	delay(DELAYWAYNE);
}

// ----------------------------------------------------------------------------------------------------
void Verificar_iButton(int cara)
{
	if( i2cFuncion.funcion == 0 )             // 
	{
		uint8_t      addr[8];      // Almacena la informacion que se lee desde el ibutton.
		char  Cod_iButton[20];
		
		//Serial.print( F("i2cFuncion.funcion: ") );  Serial.println(i2cFuncion.funcion);
		//---------------------------------------------------------------
		uint8_t  No_ds = 0;
		
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
		}
	}
}

// ----------------------------------------------------------------------------------------------------
void LoopI2C_Comunicacion()
{
	// ------------------------------------ numeracion ------------------------------------
	if((i2cFuncion.funcion == NUMERACION)&&(millis() < i2cFuncion.time))   // Solicitud de leer las numeraciones. PRINCIPAL: MEGA2560
	{
		Serial.print(F("Funcion: NUMERACION"));		Serial.println(millis());
		
		for(int surt=0; surt<Conf.Num_Surt; surt++)
		{
			Serial.print(F("***** Surtidor: "));		Serial.print(surt);

			for(int lado=0; lado<2; lado++)
			{
				Serial.print(F("***** Lado: "));	Serial.print(lado);

				for(int i=0; i<Conf.Num_Mang[surt]; i++)
				{
					Serial.print(F("***** Manguera: "));	Serial.print(i);		Serial.print(F("  ID:"));		Serial.println(IDs[surt][lado], HEX);

					getTotales(IDs[surt][lado], i+1);		// primer argumento CARA, segundo argumento Manguera.
					tmpnumeracion =0;
					
					unsigned char trama[100];

					EnviarID(IDs[surt][lado]);       // ENVIAR ID.
					res = RecibirTrama( trama );
					if(res >= 3)   VerificaRecibido( trama, res);

					if(tmpnumeracion == 0) continue;
					
					Serial.println();
					Serial.print(F("numeracion: "));	Serial.println(tmpnumeracion);
					
					numeracion[surt][lado][i] = tmpnumeracion;
				}
			}
		}

		i2cFuncion.funcion = 0;
	}
	
	// -----------------------------------------------------------------------------------------------------------------------------
	if((i2cFuncion.funcion == PRECIOS)&&(millis() < i2cFuncion.time))   // Actualizar precio de mangueras.
	{
		Serial.print(F("Funcion: PRECIOS"));		Serial.println(millis());

		for(int surt=0; surt<Conf.Num_Surt; surt++)
		{
			for(int lado=0; lado<2; lado++)
			{
				for(int mang=0; mang<4; mang++)
				{
					if(PPUArray[surt][lado][mang]!=0)
					{
						Serial.print(F("mang  : "));        Serial.println(mang);
						Serial.print(F("PPU   :"));         Serial.println(PPUArray[surt][lado][mang]);
						setPrecio(IDs[surt][lado], surt, lado, mang, PPUArray[surt][lado][mang]);
						
						PPUArray[surt][lado][mang]=0;
					}
				}
			}
		}
		
		i2cFuncion.funcion = 0;
	}
}

// ---------------------------------------------------------------
char *itostr( char *varstr, uint8_t vardato )
{
	if(vardato<10)  sprintf( varstr, "0%d", vardato );
	else      sprintf( varstr, "%d", vardato );
	return varstr;
}
