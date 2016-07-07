#include "SPI.h"
#include "PN532_SPI.h"
#include "snep.h"
#include "NdefMessage.h"
#include "Wire.h"
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <string.h>
#include "utility/debug.h"
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
										 SPI_CLOCK_DIVIDER);
#define WLAN_SSID       "hzt"           // cannot be longer than 32 characters!
#define WLAN_PASS       "ga503306"
#define WLAN_SECURITY   WLAN_SEC_WPA2
#define IDLE_TIMEOUT_MS  3000 
#define WEBSITE      "210.240.202.109" // See? No 'http://' in front of it
#define WEBPAGE      "/testard.php"

uint32_t ip;

PN532_SPI pn532spi(SPI, 4);
SNEP nfc(pn532spi);
uint8_t ndefBuf[128];

void setup() {
	Serial.begin(9600);

		if (!cc3000.begin())
		{
			Serial.println(F("Couldn't begin()! Check your wiring?"));
			while(1);
		}
	Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
		if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
			Serial.println(F("Failed!"));
			while(1);
		}

}

void loop() {
	Serial.println(F("Waiting for message from a peer"));
	int msgSize = nfc.read(ndefBuf, sizeof(ndefBuf));
	if (msgSize > 0) {
		NdefMessage msg  = NdefMessage(ndefBuf, msgSize);

		NdefRecord record = msg.getRecord(0);

		int payloadLength = record.getPayloadLength();
		byte payload[payloadLength];
		record.getPayload(payload);        

		int startChar = 0;        
		if (record.getTnf() == TNF_WELL_KNOWN && record.getType() == "T") { // text message
			// skip the language code
			startChar = payload[0] + 1;
		} else if (record.getTnf() == TNF_WELL_KNOWN && record.getType() == "U") { // URI
			startChar = 1;
		}

		String payloadAsString = "";
		for (int c = startChar; c < payloadLength; c++) {
			payloadAsString += (char)payload[c];
		}

		// Serial.print("read " + payloadAsString);
		/////wifi //////
		Serial.println(getFreeRam(), DEC);
          

	

		Serial.println(F("Connected!"));

		Serial.println(F("Request DHCP"));
		while (!cc3000.checkDHCP())
		{
			delay(100); 
		}  
		ip = cc3000.IP2U32(210,240,202,109); //指定
		cc3000.printIPdotsRev(ip);
		String sendthis = "sendthis=";

       
		sendthis.concat(payloadAsString);
		Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
               Serial.println(sendthis);
		if (www.connected()) {
			www.println("POST /testard.php HTTP/1.1");
			www.println("Host: 210.240.202.114");
			www.println("User-Agent: Arduino/1.0");
			www.println("Connection: close");
			www.println("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
			www.print("Content-Length: ");
			www.println(sendthis.length());
			www.println();
			www.println(sendthis);


		} else {
			Serial.println(F("Connection failed"));    
			return;
		}

		Serial.println(F("-------------------------------------"));
		String reg ="";
		bool begin = false;
		/* Read data until either the connection is closed, or the idle timeout is reached. */ 
		unsigned long lastRead = millis();
		while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
			while (www.available() || !begin) {
				char c = www.read();
				if (c == '{'){
					begin = true;
				}
				if (begin) reg += (c);

				if (c == '}') {
					break;
				}
				lastRead = millis();
			}
		}
		
		www.close();
  Serial.println(reg);
		 if (reg.equals("{yes}"))
                  Serial.println(F("goin"));
                  else
                  Serial.println(F("out"));
		Serial.println(F("-------------------------------------"));
		//Serial.println(F("Disconnecting"));
		//cc3000.disconnect();

	} else {
		Serial.println(F("Failed"));
        delay(500);
	}
	delay(1000);
}

