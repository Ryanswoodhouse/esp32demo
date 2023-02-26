// /*
//  * ESP32BLE控制舵机实现开关灯
//  * 新增315MHz无线遥控<------>11.23
//  * Author:猿一
//  * 2021.11.14
//  */

// #include <Arduino.h>
// #include "BluetoothSerial.h"
// #include "Servo.h"

// #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
// #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
// #endif
// void ReceiverBleMessage();

// BluetoothSerial SerialBT;
// Servo servo1,servo2;
// static const int servoPin1 = 22;
// static const int servoPin2 = 13;
// static const int OpenParameter = 70;   //开启开关舵机的角度
// // static const int CloseParameter = 5; //关闭开关舵机的角度
// static const int ResetParemeter = 0; //舵机复位角度
// String readMsg = "";
// int state=0;
// void setup(){
//   Serial.begin(115200);
//   SerialBT.begin("ESP32_KOG"); //蓝牙设备名称
//   delay(50);
//   Serial.println("The device started, now you can pair it with bluetooth!");
//   servo1.attach(
//     servoPin1,
//     Servo::CHANNEL_NOT_ATTACHED,
//     0,
//     180
//   );
//   servo1.write(ResetParemeter);
//   // servo2.attach(
//   //   servoPin2,
//   //   Servo::CHANNEL_NOT_ATTACHED,
//   //   0,
//   //   180
//   // );
//   // servo2.write(ResetParemeter);
// }
// /*
//    蓝牙串口接收
// */
// void ReceiverBleMessage(){
//   while (SerialBT.available() > 0){
//     //readMsg += char(SerialBT.read());
//     readMsg += "SWITCH";
//     delay(2);
//   }
// }
// void loop(){
//   ReceiverBleMessage();
//   if(readMsg == "SWITCH"){
//     if(state==0){
//       servo1.write(OpenParameter);
//       delay(500);
//       servo1.write(ResetParemeter);
//       Serial.print("Receiver:");
//       Serial.println(readMsg);
//     }else{
//       // servo2.write(OpenParameter-3);
//       // delay(500);
//       // servo2.write(ResetParemeter);
//       // Serial.print("Receiver:");
//       // Serial.println(readMsg);
//     }
//     state=1-state;
//   }
//   readMsg="";
// }

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
// #include "common.h"

uint8_t txValue = 0;
BLEServer *pServer = NULL;                   //BLEServer指针 pServer
BLECharacteristic *pTxCharacteristic = NULL; //BLECharacteristic指针 pTxCharacteristic
bool deviceConnected = false;                //本次连接状态
bool oldDeviceConnected = false;             //上次连接状态

// See the following for generating UUIDs: https://www.uuidgenerator.net/
#define SERVICE_UUID "12a59900-17cc-11ec-9621-0242ac130002" // UART service UUID
#define CHARACTERISTIC_UUID_RX "12a59e0a-17cc-11ec-9621-0242ac130002"
#define CHARACTERISTIC_UUID_TX "12a5a148-17cc-11ec-9621-0242ac130002"

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue(); //接收信息

        if (rxValue.length() > 0)
        { //向串口输出收到的值
            Serial.print("RX: ");
            for (int i = 0; i < rxValue.length(); i++)
                Serial.print(rxValue[i]);
            Serial.println();
        }
    }
};

void setup()
{
    Serial.begin(115200);

    // 创建一个 BLE 设备
    BLEDevice::init("ESP32_KOG");

    // 创建一个 BLE 服务
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks()); //设置回调
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // 创建一个 BLE 特征
    pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
    pTxCharacteristic->addDescriptor(new BLE2902());
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
    pRxCharacteristic->setCallbacks(new MyCallbacks()); //设置回调

    pService->start();                  // 开始服务
    pServer->getAdvertising()->start(); // 开始广播
    Serial.println(" 等待一个客户端连接，且发送通知... ");
}

void loop()
{
    // deviceConnected 已连接
    if (deviceConnected)
    {
        pTxCharacteristic->setValue(&txValue, 1); // 设置要发送的值为1
        pTxCharacteristic->notify();              // 广播
        txValue++;                                // 指针地址自加1
        delay(2000);                              // 如果有太多包要发送，蓝牙会堵塞
    }

    // disconnecting  断开连接
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);                  // 留时间给蓝牙缓冲
        pServer->startAdvertising(); // 重新广播
        Serial.println(" 开始广播 ");
        oldDeviceConnected = deviceConnected;
    }

    // connecting  正在连接
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

