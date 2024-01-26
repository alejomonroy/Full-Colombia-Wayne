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

	print_infoVenta(0);
	print_infoVenta(1);
	print_infoVenta(2);
	print_infoVenta(3);
	print_infoVenta(4);
	print_infoVenta(5);		// las 6 mangueras.
	delay(DELAYWAYNE);
}

// ----------------------------------------------------------------------------------------------------
void Verificar_iButton(int cara)
{
	if( i2cFuncion.funcion == 0 )             // 
	{
		byte      addr[8];      // Almacena la informacion que se lee desde el ibutton.
		char  Cod_iButton[20];
		
		//Serial.print( F("i2cFuncion.funcion: ") );  Serial.println(i2cFuncion.funcion);
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
		
//			Wire.beginTransmission(ARD_MEGA2560);
//			if( (cara==0)||(cara==2) )  Wire.write( strIbutton1 );    // 18
//			if( (cara==1)||(cara==3) )  Wire.write( strIbutton1 );    // 18
//			Wire.endTransmission();
		
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
			char		strI2C[100];
			strI2C[0]=0;
			char  *tmpstr1;
			tmpstr1 = (char*)(&ibutton);
			for(i=0; i<sizeof(ibutton); i++)
			{
				char chari2c = tmpstr1[i];
			
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
			
//				Wire.beginTransmission(ARD_MEGA2560);
//				Wire.write(bufI2C);
//				Wire.endTransmission();
			
				delay(7);
			
				i++;
			}
			
			// ------------------------------------
//			Wire.beginTransmission(ARD_MEGA2560);
//			Wire.write("END");            // 3
//			Wire.endTransmission();
			Serial.println( F("END") );
		
		}
	}
	//Serial.print( F("iButton: ") );   Serial.println(millis());

}

// ----------------------------------------------------------------------------------------------------
void LoopI2C_Comunicacion()
{
	// ------------------------------------ numeracion ------------------------------------
	if((i2cFuncion.funcion == NUMERACION)||(millis() < i2cFuncion.time))   // Solicitud de enviar venta. PRINCIPAL: MEGA2560
	{
		Serial.print(F("(2). DatosI2C: "));    Serial.print(DatosI2C);    Serial.print(F(" - "));    Serial.println(millis());
		
		for(int j=0; j<Conf.Num_Caras; j++)			// Cada cara.
		{
			byte Num_Mang = 0;
			if((j==0)||(j==2)) Num_Mang= Conf.Num_Mang_1;
			if((j==1)||(j==3)) Num_Mang= Conf.Num_Mang_2;
			
			for(int i=1; i<=Num_Mang; i++)				// Cada manguera. (1, 2, 3)
			{
				Serial.print(F("Manguera: "));	Serial.println(i);

				Serial.print(F("  Cara:"));		Serial.println(IDs[j], HEX);
				getTotales(IDs[j], i);		// primer argumento CARA, segundo argumento Manguera.
				tmpnumeracion =0;
				
				unsigned char trama[100];
        
				EnviarID(IDs[j]);       // ENVIAR ID.
				res = RecibirTrama( trama );
				if(res >= 3)   VerificaRecibido( trama, res);

				if(tmpnumeracion == 0) continue;
				
				Serial.println();
				Serial.print(F("i-1: "));	Serial.print(i-1);	Serial.print(F(", j: "));	Serial.println(j);
				Serial.print(F("numeracion: "));	Serial.println(tmpnumeracion);
				
				int index = j*3 + i;
				venta[index].Numeracion = tmpnumeracion;
			}
		}
		
		Serial.println(F("Numeracion..."));
		
		i2cFuncion.funcion = 0;
	} // FIN ENVIAR VENTA.
	
	
	if( i2cFuncion.funcion == 6 )
	{
		i2cFuncion.funcion = 0;
	} // */
	
	if((i2cFuncion.funcion == PRECIOS)||(millis() < i2cFuncion.time))   // Actualizar precio de mangueras.
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
		
		i2cFuncion.funcion = 0;
	} // 
	
}

// ---------------------------------------------------------------
char *itostr( char *varstr, byte vardato )
{
	if(vardato<10)  sprintf( varstr, "0%d", vardato );
	else      sprintf( varstr, "%d", vardato );
	return varstr;
}
