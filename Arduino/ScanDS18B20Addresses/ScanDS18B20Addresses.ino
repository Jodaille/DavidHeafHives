/**
 * Lecture des adresses des capteurs
 * de température DS18B20
 */

/** 
 *  Nous utilisons la librairie OneWire
 *  Elle doit être présente dans le répertoire libraries
 *  situé dans le répertoire des croquis/sketchs
 *  voir dans le menu Préférences
 */
#include <OneWire.h>

// Le fil jaune des données est relié au port suivant:
const int dsPin = 8;


OneWire  ds(dsPin);

void setup(void) {
  Serial.begin(9600);
  /**
   * Nous appelons la fonction listant les adresses
   * des capteurs en présence
   */
  getDeviceAddress();
;
}

/**
 * Fonction de scan des adresses des capteurs 
 */
void getDeviceAddress(void) {
  byte i;
  byte adresse[8];
  int countS = 1;
  Serial.println("fetching sensors addresses...\n\r");
  /* initiate a search for the OneWire object we created and read its value into
  addr array we declared above*/
  
  while(ds.search(adresse)) {
    Serial.print("DeviceAddress temperature");
    Serial.print(countS);Serial.print(" {");
    //read each byte in the address array
    for( i = 0; i < 8; i++) {
      
      Serial.print("0x");
      if (adresse[i] < 16) {
        
        Serial.print('0');
      }
      // print each byte in the address array in hex format
      Serial.print(adresse[i], HEX);
      if (i < 7) {
        Serial.print(", ");
      }
      else
      {
        Serial.print("}; ");
        }
    }
    // a check to make sure that what we read is correct.
    if ( OneWire::crc8( adresse, 7) != adresse[7]) {
        Serial.print("CRC is not valid!\n");
        return;
    }
    countS++;
    Serial.println();
  }
  ds.reset_search();
  return;
}

void loop(void) {
  // do nothing
}
