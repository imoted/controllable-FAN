#include <ESP8266WiFi.h>
extern "C" {
#include "user_interface.h"
}
#define ID 0x01

WiFiClient client;
const int httpPort = 23;

//char toSSID[] = "HUMAX-24EB5";
//char ssidPASSWD[] = "MGN5N5XWdmG3X";

char toSSID[] = "AirPort53154";
char ssidPASSWD[] = "3493249157475";
const char* host = "192.168.0.5";  //  processingのサーバーのIPアドレス

uint16_t ADC_Value = 0;
uint16_t PWMDuty;
uint8_t btx[20];
uint8_t i=0;
uint8_t cc;
uint8_t fbuf;
uint8_t buf0,buf1,buf2,buf3,buf4,buf5 ;
uint16_t datasum,sum;
uint8_t disp_sw=1;

void setup() {
  //デバッグ用にシリアルを開く
  Serial.begin(115200);
  Serial.println("PG start");

  //WiFiに繋がったらLEDを点灯させるので、そのピンをOUTPUTに設定して、LOWに。
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);

  //WiFiクライアントモード設定
  WiFi.mode(WIFI_STA);

  //WiFiを繋ぐ前に、WiFi状態をシリアルに出力
  WiFi.printDiag(Serial);

  //WiFiの設定を入れる。IPは、デフォルトがDHCPのようで、IPが共有されるときはWiFiのアクセス情報だけでOK
  WiFi.begin(toSSID, ssidPASSWD);

  // 固定IPではネットワークステータスが上手く動かないので、今は使えなさそう。本家にバグ登録はされているらしい。
  //  WiFi.config(ip, gateway, subnet);

  //接続が確立するまで、・・・を表示
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    WiFi.printDiag(Serial);
  }

  //改行して繋がったことをシリアルで伝える。
  Serial.println("");
  Serial.println("WiFi connected");
  
  //WiFiの状態を表示
//  WiFi.printDiag(Serial);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // 無線LAN接続OK LED点灯
  digitalWrite(5, HIGH);
 client.connect(host, httpPort);

}


void loop() {
  
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(5, LOW);      // 無線LAN接続NG LED消灯
  } else {
    digitalWrite(5, HIGH );     //コネクションOKなので、点灯
  }

  if (!client.connected()) {
     client.connect(host, httpPort);
  }


ADC_Value = system_adc_read();  //AD変換実行
btx[1]=((ADC_Value >>8) & 0x00ff) | 0xa0;
btx[2]=ADC_Value & 0x00ff;

client.write(char(btx[1]));
client.write(char(btx[2]));


if(client.available() != 0){
     cc = client.read();
     if(disp_sw==1){
      Serial.print(cc,HEX);
      }
     switch(fbuf){
      case 0: if(cc == 0x7e){fbuf++; sum = cc;  break;}else{fbuf=0;break; }
      case 1: if(cc == ID){fbuf++; sum += cc;  break;}else{fbuf=0;break; }
      case 2: buf0=cc; sum += cc; fbuf++;  break;
      case 3: buf1=cc; sum += cc; fbuf++;  break;
      case 4: buf2=cc; sum += cc; fbuf++;  break;
      case 5: buf3=cc; sum += cc; fbuf++;  break;
      case 6: buf4=cc; sum += cc; fbuf++;  break;
      case 7: buf5=cc; sum += cc; fbuf++;  break;                        
      
      case 8: datasum = sum%256; Serial.print("datasum =");  Serial.println(datasum);
      if( datasum == cc){  //GPIO 0 2 5 15 は使えない。 0 2 はBOOTに関係するピンで起動時H にボード内部で10kでpull up 15は起動時pulldown 10k 。 5はPWMが出ない。仕様？
        analogWrite(13, map(buf0,0,100,0,1024));  //
        analogWrite(15, map(buf1,0,100,0,1024));  //
        analogWrite(4, map(buf2,0,100,0,1024));  //
        analogWrite(12, map(buf3,0,100,0,1024));  //
        analogWrite(14, map(buf4,0,100,0,1024));  //
        analogWrite(16, map(buf5,0,100,0,1024));  //
        
        Serial.println("change PWM Duty!");
      }
      fbuf=0;
      break;

     }
}

}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
// 2016.07.03　ESP201だからなのか、10秒に一回ぐらいWIFIが切断される
// 2017.01.19  WIFI切れるのは、WIFIではなく、システムがoverflowで落ちてwdtがかかっていた。
//　また ADCの変換スピードは、おそらく10Hz程度。　client.connectをloopに入れると毎回接続しにいくので、loopがさらに遅れる。
// Serial 切っても変化なし。

//  if (!client.connect(host, httpPort)) {  //これいれると、excepmtion (29)で落ちる。　
//    Serial.println("connection failed");
//    return;
//  }
// Exception(29) ってなに
// https://github.com/esp8266/Arduino/blob/master/doc/exception_causes.md によれば
// 29 StoreProhibitedCause: A store referenced a page mapped with an attribute that does not permit stores Region Protection or MMU
// 書き込んではいけない領域に書き込もうとしたみたいですね。


