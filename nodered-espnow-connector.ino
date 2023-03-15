#include <ESP8266WiFi.h>
#include <espnow.h>

static char dataBuffer[600];

//demo frame:
//len, 6by mac, 1-255by payload
//5 FF FF FF FF FF FF B4 E6 2D 42 C0

static uint8_t payload_len;
static uint8_t receiver[6];
static uint8_t payload[250];

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (sendStatus == 0){
    Serial.println("Success");
  }
  else{
    Serial.println("Fail");
  }
}

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  uint8_t myData[len];
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print('{');
  Serial.print(len);
  Serial.print('|');
  for (int i = 0; i < 6; i++){ Serial.print(mac[i], HEX); Serial.print(' '); }
  Serial.print('|');
  for (int i = 0; i < len; i++){ Serial.print(myData[i], HEX); Serial.print(' '); }
  Serial.println('}');
}

void setup() {
  Serial.begin(230400);
  Serial.setTimeout(100);

  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.println();

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

}

void loop() {
  
  if(Serial.available() > 0)
  {
    //reset buffer
     memset(dataBuffer, 0, 600*sizeof(char));
    //fill buffer
    if( Serial.readBytesUntil('\n', dataBuffer, 600) != 0)
    {
      char* token = strtok(dataBuffer, " \n");
      uint16_t tok_idx = 0;
      while(token != NULL){

        //some payload lengths are reserved, like 0 (should be 251-255 bc max payload is 250)
        //0 means add peer as slave
        if (payload_len == 0 && tok_idx == 7){
          esp_now_add_peer(receiver, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
          while(Serial.available() > 0) Serial.read(); //empty serial
          return;
        }

        //parse data
        if (tok_idx == 0){
          payload_len = strtol(token, NULL, 10);
        } else if (tok_idx <= 6) {
          receiver[tok_idx - 1] = strtol(token, NULL, 16);
        } else if (tok_idx <= 262) {
          payload[tok_idx - 7] = strtol(token, NULL, 16);
        }
        
        //continue token
        token = strtok (NULL, " \n");
        tok_idx++;
      }

      esp_now_send(receiver, (uint8_t *) &payload, payload_len);
    }
    while(Serial.available() > 0) Serial.read(); //empty serial 
  }
}
