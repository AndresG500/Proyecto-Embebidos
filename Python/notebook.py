
def leer_sd_windows():
    import os
    
    print("🔍 Buscando unidades disponibles...")
    print("\nLetras de unidad detectadas:")
    
    from string import ascii_uppercase
    available_drives = []
    
    for letter in ascii_uppercase:
        drive = f"{letter}:\\"
        if os.path.exists(drive):
            try:
                if os.path.ismount(drive) or os.path.exists(drive):
                    available_drives.append(letter)
                    print(f"  {letter}: - {drive}")
            except:
                pass
    
    if not available_drives:
        print("❌ No se encontraron unidades")
        return
    
    letra = input("\n📝 Ingresa la letra de la SD (ejemplo: E): ").strip().upper()
    
    if letra not in available_drives:
        print(f"❌ Unidad {letra}: no disponible")
        return
    
    dispositivo = f"\\\\.\\{letra}:"
    
    print(f"\n📊 Leyendo datos desde {dispositivo}")
    print("=" * 60)
    
    lineas_guardadas = []
    
    try:
        with open(dispositivo, 'rb') as f:
            for i in range(50): 
                sector = 1000 + i
                offset = sector * 512
                
                f.seek(offset)
                data = f.read(512)
                
                # Extraer texto ASCII
                line = bytearray()
                datos_encontrados = False
                
              
                for j, byte in enumerate(data[:100]):
                    if byte == 0xFF:
                        break  
                    if byte == 0x00:
                        if len(line) > 0:
                            break  
                        else:
                            continue  
                    if 32 <= byte <= 126 or byte in [10, 13]:
                        line.append(byte)
                        datos_encontrados = True
                
                if line and datos_encontrados:
                    try:
                        text = line.decode('ascii', errors='ignore').strip()
                        if text and len(text) > 3: 
                            print(f"Sector {sector:4d}: {text}")
                            lineas_guardadas.append(text)
                    except:
                        pass
                else:
                    if data[:100].count(0xFF) > 90:
                        if i == 0:
                            print(f"⚠️ Sector {sector} está vacío (0xFF)")
                            print("   ¿El PIC realmente guardó datos?")
                        break
                    elif i > 0:
                        print(f"\n✓ Fin de datos en sector {sector-1}")
                        break
        
        print("=" * 60)
        print("✓ Lectura completada")
        
        # GUARDAR EN ARCHIVO TXT
        if lineas_guardadas:
            nombre_archivo = f"datos_sd_{letra}.txt"
            with open(nombre_archivo, 'w', encoding='utf-8') as f:
                f.write('\n'.join(lineas_guardadas))
            
            print(f"\n📄 ARCHIVO GENERADO: {nombre_archivo}")
            print(f"   Total de líneas: {len(lineas_guardadas)}")
            print(f"   Ubicación: {os.path.abspath(nombre_archivo)}")
        else:
            print("\n⚠️ No se encontraron datos para guardar")
        
    except PermissionError:
        print("\n⚠️ Error de permisos. Ejecuta PowerShell como Administrador:")
        print(f"   python leer_sd_directo.py")
        print("\nO usa el método alternativo:")
        leer_archivos_sd(letra)
    except Exception as e:
        print(f"❌ Error: {e}")
        print("\nIntentando método alternativo...")
        leer_archivos_sd(letra)


def leer_archivos_sd(letra):
    """Busca archivos en la SD y los guarda en TXT"""
    import os
    
    drive = f"{letra}:\\"
    print(f"\n📁 Buscando archivos en {drive}")
    
    todas_lineas = []
    
    try:
        for root, dirs, files in os.walk(drive):
            for file in files:
                filepath = os.path.join(root, file)
                print(f"\n📄 Archivo: {filepath}")
                
                try:
                    with open(filepath, 'rb') as f:
                        data = f.read()
                        
                        # Intentar decodificar como texto
                        try:
                            # Decodificar y limpiar caracteres ÿ
                            text = data.decode('latin-1')
                            lineas = text.split('\n')
                            
                            for linea in lineas:
                                linea_limpia = linea.replace('ÿ', '').strip()
                                if linea_limpia and len(linea_limpia) > 3:
                                    print(f"  {linea_limpia}")
                                    todas_lineas.append(linea_limpia)
                        except:
                            print("  [Archivo binario - omitido]")
                except:
                    pass
        
        # GUARDAR TODO EN UN TXT
        if todas_lineas:
            nombre_archivo = f"datos_sd_{letra}_archivos.txt"
            with open(nombre_archivo, 'w', encoding='utf-8') as f:
                f.write('\n'.join(todas_lineas))
            
            print(f"\n📄 ARCHIVO GENERADO: {nombre_archivo}")
            print(f"   Total de líneas: {len(todas_lineas)}")
            print(f"   Ubicación: {os.path.abspath(nombre_archivo)}")
        else:
            print("\n⚠️ No se encontraron datos para guardar")
            
    except Exception as e:
        print(f"❌ Error: {e}")


def leer_imagen_sd(archivo):
    """Lee desde una imagen .bin de la SD y guarda en TXT"""
    print(f"📊 Leyendo imagen: {archivo}")
    print("=" * 60)
    
    lineas_guardadas = []
    
    try:
        with open(archivo, 'rb') as f:
            for i in range(50):
                sector = 1000 + i
                offset = sector * 512
                
                f.seek(offset)
                data = f.read(512)
                
                line = bytearray()
                for byte in data:
                    if byte == 0x00 or byte == 0xFF:
                        break
                    if 32 <= byte <= 126 or byte in [10, 13]:
                        line.append(byte)
                
                if line:
                    try:
                        text = line.decode('ascii').strip()
                        if text:
                            print(f"Sector {sector:4d}: {text}")
                            lineas_guardadas.append(text)
                    except:
                        pass
                else:
                    if i > 0:
                        print(f"\n✓ Fin de datos en sector {sector-1}")
                        break
        
        print("=" * 60)
        print("✓ Lectura completada")
        
        # GUARDAR EN TXT
        if lineas_guardadas:
            import os
            nombre_base = os.path.splitext(archivo)[0]
            nombre_archivo = f"{nombre_base}_datos.txt"
            
            with open(nombre_archivo, 'w', encoding='utf-8') as f:
                f.write('\n'.join(lineas_guardadas))
            
            print(f"\n📄 ARCHIVO GENERADO: {nombre_archivo}")
            print(f"   Total de líneas: {len(lineas_guardadas)}")
            print(f"   Ubicación: {os.path.abspath(nombre_archivo)}")
        else:
            print("\n⚠️ No se encontraron datos para guardar")
            
    except Exception as e:
        print(f"❌ Error: {e}")


if __name__ == "__main__":
    import sys
    import os
    
    print("=" * 60)
    print("  LECTOR DE DATOS SD - PIC16F887")
    print("  Temperatura y Humedad (DHT22 + DS3231)")
    print("  >> GUARDA AUTOMÁTICAMENTE EN TXT <<")
    print("=" * 60)
    
    if len(sys.argv) > 1:
        # Si se pasa un archivo como argumento
        archivo = sys.argv[1]
        if os.path.exists(archivo):
            leer_imagen_sd(archivo)
        else:
            print(f"❌ Archivo no encontrado: {archivo}")
    else:
        # Modo interactivo (Windows)
        if os.name == 'nt':
            leer_sd_windows()
        else:
            print("ℹ️ En Linux, usa:")
            print("   sudo python leer_sd_directo.py /dev/sdX")
            print("\nO crea primero una imagen:")
            print("   sudo dd if=/dev/sdX of=sd_image.bin bs=512 count=2000")
            print("   python leer_sd_directo.py sd_image.bin")
    
    input("\n\nPresiona Enter para salir...")