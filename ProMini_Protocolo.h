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
void  bzero(char *str, int tam){	for(int i=0; i<tam; i++) str[i]=0;	}

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

	print_infoVenta(0, 0);
	print_infoVenta(0, 1);
	print_infoVenta(1, 0);
	print_infoVenta(1, 1);
	print_infoVenta(2, 0);
	print_infoVenta(2, 1);   // los 6 lados.
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
	if((i2cFuncion.funcion == NUMERACION)||(millis() < i2cFuncion.time))   // Solicitud de leer las numeraciones. PRINCIPAL: MEGA2560
	{
		Serial.print(F("(2). DatosI2C: "));    Serial.print(DatosI2C);    Serial.print(F(" - "));    Serial.println(millis());
		
		for(int surt=0; surt<Conf.Num_Surt; surt++)
		{
			Serial.print(F("***** Surtidor: "));		Serial.println(surt);

			for(int lado=0; lado<2; lado++)
			{
				Serial.print(F("***** Lado: "));	Serial.print(lado);

				for(int i=0; i<Conf.Num_Mang; i++)
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
	} // FIN ENVIAR VENTA.
	
	// -----------------------------------------------------------------------------------------------------------------------------
	if( i2cFuncion.funcion == 6 )
	{
		i2cFuncion.funcion = 0;
	}
	
	// -----------------------------------------------------------------------------------------------------------------------------
	if((i2cFuncion.funcion == PRECIOS)||(millis() < i2cFuncion.time))   // Actualizar precio de mangueras.
	{
		Serial.println(F("CAMBIANDO PRECIOS..."));
		for(int i=0; i<6; i++)
		{
			if(PPUArray[i]!=0)
			{
				Serial.print(F("i  : "));        Serial.println(i);
//				Serial.print(F("PPU:"));         Serial.println(PPUArray[i]);@@@@@@@@
				if(i==0) setPrecio(IDs[0], 1, PPUArray[i]);
				if(i==1) setPrecio(IDs[0], 2, PPUArray[i]);
				if(i==2) setPrecio(IDs[2], 1, PPUArray[i]);
		
				if(i==3) setPrecio(IDs[1], 1, PPUArray[i]);
				if(i==4) setPrecio(IDs[1], 2, PPUArray[i]);
				if(i==5) setPrecio(IDs[3], 1, PPUArray[i]);
			
//				PPUArray[i]=0;@@@@@@@@
			}
		}
		
		i2cFuncion.funcion = 0;
	} // 
	
}

// ---------------------------------------------------------------
char *itostr( char *varstr, uint8_t vardato )
{
	if(vardato<10)  sprintf( varstr, "0%d", vardato );
	else      sprintf( varstr, "%d", vardato );
	return varstr;
}

