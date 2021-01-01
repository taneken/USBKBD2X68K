//----------------------------------------------------------------------
// USBKBD2X68K for Arduino uno(ATMEGA328p compatible)
// X68000のキーボードコネクタにUSBキーボードとUSBマウスを接続
//----------------------------------------------------------------------
// 2020.11.09 Ver.0.1   とりあえず動いた版
// 2020.12.30 Ver.0.2   キーリピート間隔の実装（しかし実機と挙動が異なる）
// 2021.01.02 Ver.0.3   MOUSE_MODEでマウスを使用しないときはOFF出来るようにした
//----------------------------------------------------------------------
// このスケッチのコンパイルには以下のライブラリが必要です.
//  ・USB_Host_Shield_2.0 (https://github.com/felis/USB_Host_Shield_2.0)
//  ・MsTimer2 (http://playground.arduino.cc/Main/MsTimer2)
//----------------------------------------------------------------------
// キーアサイン
//  ・F11      -> かな(0x5a)
//  ・F12      -> ローマ字(0x5b)
//  ・LeftWin  -> ひらがな(0x5f)
//  ・LeftAlt  -> XF1(0x55)
//  ・無変換    -> XF2(0x56)
//  ・変換      -> XF3(0x57)
//  ・カタカナ   -> XF4(0x58)
//  ・RightAlt  -> XF5(0x59)
//  ・RgihtWin  -> N/A
//  ・Menu      -> OPT.1(0x72)
//  ・RightCtrl -> OPT.2(0x73)
//  ・END       -> UNDO(0x3a)
//  ・ScrollLock-> HELP(0x54)
//  ・Pause     -> BREAK(0x61)
//  ・PrintScr  -> COPY(0x62)
//  ・NumLock   -> CLR(0x3f)
// 未アサイン
//  ・          -> 記号入力(0x52)
//  ・          -> コード入力(0x5c)
//  ・          -> 登録(0x53)
//----------------------------------------------------------------------
// キーボードコネクタ配線(本体側)
//
//   7 6 5
//  4     3
//   2   1
//
//  本体側             Arduino側
//  -------------------------------------------
//  1:Vcc2 5V(out) -> 5V
//  2:MSDATA(in)   <- TX(1)
//  3:KEYRxD(in)   <- A0(14) softwareSerial TX 
//  4:KEYTxD(out)  -> A1(15) softwareSerial RX
//  5:READY(out)
//  6:REMOTE(in)
//  7:GND(--)      -- GND
//
//----------------------------------------------------------------------

#define MYDEBUG      0

#define MOUSE_MODE   0            // 0:未使用 1:使用（ハードウェアシリアル）

#include <BTHID.h>
#include <hidboot.h>
#include <usbhub.h>
#include <MsTimer2.h>
#include <SoftwareSerial.h>

#include "keymap.h"

SoftwareSerial KBDSerial(15,14); // RX(KEYTxD/MSCTRL), TX(KEYRxD)

#define LOBYTE(x) ((char*)(&(x)))[0]
#define HIBYTE(x) ((char*)(&(x)))[1]

boolean LeftButton = false;         // マウス左ボタン
boolean MidButton = false;          // マウス真ん中ボタン
boolean RightButton = false;        // マウス右ボタン
byte dx;                            // マウスX軸
byte dy;                            // マウスY軸
uint8_t MSCTRL;
uint8_t oldCTRL;
byte MSDATA;

// キーリピートの定義
#define REPEATTIME      5   // キーを押し続けて、REP_INTERVALxREPEATTIMEmsec後にリピート開始
#define EMPTY           0   // リピート管理テーブルが空状態
#define MAXKEYENTRY     6   // リピート管理テーブルサイズ
//#define REP_INTERVAL    50 // リピート間隔
uint8_t REP_INTERVAL = 50;

uint8_t keyentry[MAXKEYENTRY];    // リピート管理テーブル
uint8_t repeatWait[MAXKEYENTRY];  // リピート開始待ち管理テーブル

//-----------------------------------------------------------------------------

//
// HIDキーボード レポートパーサークラスの定義
//
class KbdRptParser : public KeyboardReportParser {
  protected:
    virtual uint8_t HandleLockingKeys(USBHID *hid, uint8_t key);
    virtual void OnControlKeysChanged(uint8_t before, uint8_t after);
    virtual void OnKeyDown(uint8_t mod, uint8_t key);
    virtual void OnKeyUp(uint8_t mod, uint8_t key);
    virtual void OnKeyPressed(uint8_t key) {};
};

//
// HIDマウス レポートパーサークラスの定義
//
class MouseRptParser : public MouseReportParser {
  protected:
    void OnMouseMove  (MOUSEINFO *mi);
    void OnLeftButtonUp (MOUSEINFO *mi);
    void OnLeftButtonDown (MOUSEINFO *mi);
    void OnRightButtonUp  (MOUSEINFO *mi);
    void OnRightButtonDown  (MOUSEINFO *mi);
    void OnMiddleButtonUp (MOUSEINFO *mi);
    void OnMiddleButtonDown (MOUSEINFO *mi);
};

//-----------------------------------------------------------------------------

USB     Usb;
USBHub  Hub1(&Usb);
USBHub  Hub2(&Usb);
USBHub  Hub3(&Usb);
USBHub  Hub4(&Usb);

BTD     Btd(&Usb);
BTHID   bthid(&Btd);
//BTHID   bthid(&Btd, PAIR);
//BTHID   bthid(&Btd, PAIR, "0000");

HIDBoot<3>    HidComposite(&Usb);
HIDBoot<1>    HidKeyboard(&Usb);
HIDBoot<2>    HidMouse(&Usb);
//HIDBoot<USB_HID_PROTOCOL_KEYBOARD | USB_HID_PROTOCOL_MOUSE> HidComposite(&Usb);
//HIDBoot<USB_HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);
//HIDBoot<USB_HID_PROTOCOL_MOUSE>    HidMouse(&Usb);

KbdRptParser keyboardPrs;
MouseRptParser MousePrs;

//-----------------------------------------------------------------------------

void MouseRptParser::OnMouseMove(MOUSEINFO *mi)
{
/*    Serial.print("dx=");
    Serial.print(mi->dX, DEC);
    Serial.print(" dy=");
    Serial.println(mi->dY, DEC); */
    dx = mi->dX;
    dy = mi->dY;

};
void MouseRptParser::OnLeftButtonUp (MOUSEINFO *mi) {
//    Serial.println("L Butt Up");
    LeftButton = false;
};
void MouseRptParser::OnLeftButtonDown (MOUSEINFO *mi) {
//    Serial.println("L Butt Dn");
    LeftButton = true;
};
void MouseRptParser::OnRightButtonUp  (MOUSEINFO *mi) {
//    Serial.println("R Butt Up");
    RightButton = false;
};
void MouseRptParser::OnRightButtonDown  (MOUSEINFO *mi) {
//    Serial.println("R Butt Dn");
    RightButton = true;
};
void MouseRptParser::OnMiddleButtonUp (MOUSEINFO *mi) {
//    Serial.println("M Butt Up");
    MidButton = false;
};
void MouseRptParser::OnMiddleButtonDown (MOUSEINFO *mi) {
//    Serial.println("M Butt Dn");
    MidButton = true;
};

//--------------------------------------------------------------------------


uint8_t classType = 0;      
uint8_t subClassType = 0;

//
// keyboard 1 byte send (software serial)
//
void byte_send(char code) {
  KBDSerial.listen();
  KBDSerial.write(code);
}

//
// リピート管理テーブルのクリア
//
void claerKeyEntry() {
 for (uint8_t i=0; i <MAXKEYENTRY; i++)
    keyentry[i] = EMPTY;
}

//
// リピート管理テーブルにキーを追加
//
void addKey(uint8_t key) {
 for (uint8_t i=0; i <MAXKEYENTRY; i++) {
  if (keyentry[i] == EMPTY) {
    keyentry[i] = key;  
    repeatWait[i] = REPEATTIME;
    break;
  }
 }
}

//
// リピート管理テーブルからキーを削除
//
void delKey(uint8_t key) {
 for (uint8_t i=0; i <MAXKEYENTRY; i++) {
  if (keyentry[i] == key) {
    keyentry[i] = EMPTY;
    break;
  }
 }  
}

// リピート処理(タイマー割り込み処理から呼ばれる)
void sendRepeat() {
  // HID Usage ID から PS/2 スキャンコード に変換
  uint8_t key;
  uint8_t code;
  
  for (uint8_t i=0; i < MAXKEYENTRY; i++) {
    if (keyentry[i] != EMPTY) {
      key = keyentry[i]; 
      if (repeatWait[i] == 0) {
        code = pgm_read_byte(&(keytable[key]));
//        Serial.print(F("keytable="));Serial.print(key,HEX);Serial.print(F(" code=")); Serial.println(code,HEX);
        byte_send(code);
    } else {
        repeatWait[i]--;          
      }
    }
  }
}

//
// ロックキー（NumLock/CAPSLock/ScrollLock)ハンドラ
//
uint8_t KbdRptParser::HandleLockingKeys(USBHID *hid, uint8_t key) {
  if (classType == USB_CLASS_WIRELESS_CTRL) {
    uint8_t old_keys = kbdLockingKeys.bLeds;  
    switch (key) {
//      case UHS_HID_BOOT_KEY_NUM_LOCK:
//        kbdLockingKeys.kbdLeds.bmNumLock = ~kbdLockingKeys.kbdLeds.bmNumLock;
//        break;
      case UHS_HID_BOOT_KEY_CAPS_LOCK:
        kbdLockingKeys.kbdLeds.bmCapsLock = ~kbdLockingKeys.kbdLeds.bmCapsLock;
        break;
//      case UHS_HID_BOOT_KEY_SCROLL_LOCK:
//        kbdLockingKeys.kbdLeds.bmScrollLock = ~kbdLockingKeys.kbdLeds.bmScrollLock;
//        break;
    }
    if (old_keys != kbdLockingKeys.bLeds && hid) {
      BTHID *pBTHID = reinterpret_cast<BTHID *> (hid); // A cast the other way around is done in BTHID.cpp
      pBTHID->setLeds(kbdLockingKeys.bLeds); // Update the LEDs on the keyboard
    }
  } else {
    return KeyboardReportParser::HandleLockingKeys(hid, key);   
  }
  return 0;
}


//
// キー押しハンドラ
// 引数
//  mod : コントロールキー状態
//  key : HID Usage ID 
//
void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key) {
  MsTimer2::stop();

  uint8_t code;
  code = pgm_read_byte(&(keytable[key]));
  if (code == 0x00)
    return;

#if MYDEBUG == 1
  Serial.print(F("keytable="));Serial.print(key,HEX);Serial.print(F(" code=")); Serial.println(code,HEX);
#endif
  byte_send(code);
  addKey(key);
  MsTimer2::start();
}

//
// キー離し ハンドラ
// 引数
//  mod : コントロールキー状態
//  key : HID Usage ID 
//
void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key) {
  MsTimer2::stop();
  uint8_t code;
  code = pgm_read_byte(&(keytable[key]));
  if (code == 0x00)
    return;

  code |= 0x80; // 離すときは 1000 0000 にビットを立てる
#if MYDEBUG == 1
//  Serial.print(F("keytable="));Serial.print(key,HEX);Serial.print(F(" code=")); Serial.println(code,HEX);
#endif
  byte_send(code);
  delKey(key);
  MsTimer2::start();
}

//
// コントロールキー変更ハンドラ
// SHIFT、CTRL、ALT、GUI(Win)キーの処理を行う
// 引数 before : 変化前のコード USB Keyboard Reportの1バイト目
//      after  : 変化後のコード USB Keyboard Reportの1バイト目
//
void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {
  MODIFIERKEYS beforeMod;
  *((uint8_t*)&beforeMod) = before;

  MODIFIERKEYS afterMod;
  *((uint8_t*)&afterMod) = after;

  uint8_t code;

  // 左Ctrlキー
  if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl) {
    code = 0x71;
    if (afterMod.bmLeftCtrl) {
      byte_send(code);      // 左Ctrlキーを押した
    } else {
      code |= 0x80;
      byte_send(code);      // 左Ctrlキーを離した
    } 
  }

  // 左Shiftキー
  if (beforeMod.bmLeftShift != afterMod.bmLeftShift) {
    code = 0x70;
    if (afterMod.bmLeftShift) {
      byte_send(code);      // 左Shiftキーを押した
    } else {
      code |= 0x80;
      byte_send(code);      // 左Shiftキーを離した
    }
  }

  // 左Altキー(XF1)
  if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt) {
    code = 0x55;
    if (afterMod.bmLeftAlt) {
      byte_send(code);      // 左Altキーを押した
    } else {
      code |= 0x80;
      byte_send(code);      // 左Altキーを離した
    }
  }

  // 左GUIキー(Winキー)(ひらがな)
  if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI) {
    code = 0x5f;
    if (afterMod.bmLeftGUI) {
      byte_send(code);      // 左Winキーを押した
    } else {
      code |= 0x80;
      byte_send(code);      // 左Winキーを離した
    }
  }

  // 右Ctrlキー(OPT.2)
  if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl) {
    code = 0x73;
    if (afterMod.bmRightCtrl) {
      byte_send(code);      // 右Ctrlキーを押した
    } else {
      code |= 0x80;
      byte_send(code);      // 右Ctrlキーを離した
    } 
  }

  // 右Shiftキー
  if (beforeMod.bmRightShift != afterMod.bmRightShift) {
    code = 0x70;
    if (afterMod.bmRightShift) {
      byte_send(code);      // 右Shiftキーを押した
    } else {
      code |= 0x80;
      byte_send(code);      // 右Shiftキーを離した
    }
  }

  // 右Altキー(XF5)
  if (beforeMod.bmRightAlt != afterMod.bmRightAlt) {
    code = 0x59;
    if (afterMod.bmRightAlt) {
      byte_send(code);      // 右Altキーを押した
    } else {
      code |= 0x80;
      byte_send(code);      // 右Altキーを離した
    }
  }

  // 右GUIキー(opt1)
  if (beforeMod.bmRightGUI != afterMod.bmRightGUI) {
    code = 0x72;
    if (afterMod.bmRightGUI) {
      byte_send(code);      // 右Winキーを押した
    } else {
      code |= 0x80;
      byte_send(code);      // 右Winキーを離した
    }
  }
}

//
// インターフェースクラスの取得
//
uint8_t getIntClass(byte& intclass, byte& intSubClass ) {
  uint8_t buf[ 256 ];
  uint8_t* buf_ptr = buf;
  byte rcode;
  byte descr_length;
  byte descr_type;
  unsigned int total_length;

  uint8_t flgFound = 0;
  
  //デスクプリタトータルサイズの取得
  rcode = Usb.getConfDescr( 1, 0, 4, 0, buf );
  LOBYTE( total_length ) = buf[ 2 ]; HIBYTE( total_length ) = buf[ 3 ];
  if ( total_length > 256 ) {
    total_length = 256;
  }
  
  rcode = Usb.getConfDescr( 1, 0, total_length, 0, buf ); 
  while ( buf_ptr < buf + total_length ) { 
    descr_length = *( buf_ptr );
    descr_type = *( buf_ptr + 1 );

    if (descr_type == USB_DESCRIPTOR_INTERFACE) {
      // 最初のインタフェースの取得
      USB_INTERFACE_DESCRIPTOR* intf_ptr = ( USB_INTERFACE_DESCRIPTOR* )buf_ptr;  
      intclass = intf_ptr->bInterfaceClass;
      intSubClass = intf_ptr->bInterfaceSubClass;
      flgFound = 1;
      break;
    }
    buf_ptr = ( buf_ptr + descr_length );    //advance buffer pointer
  }
  return ( flgFound );
}

void rep_timer() {
  MsTimer2::set(REP_INTERVAL, sendRepeat); 
  MsTimer2::start();
}


void setup() {

#if MOUSE_MODE == 1
  Serial.begin(4800,SERIAL_8N2);  //MSDATA送信用
#endif
  KBDSerial.begin(2400);
//  Serial.println("Self Test OK.");

// USB初期化
  if (Usb.Init() == -1) {
//    Serial.println(F("OSC did not start."));
    while (1); // Halt    
  }
  delay( 200 );
//  Serial.println(F("BT Start."));
//  bthid.SetReportParser(KEYBOARD_PARSER_ID, &keyboardPrs);
  bthid.SetReportParser(MOUSE_PARSER_ID, &MousePrs);
  bthid.setProtocolMode(USB_HID_BOOT_PROTOCOL); // Boot Protocol Mode

//  Serial.println(F("HID Start."));
  HidComposite.SetReportParser(0, &keyboardPrs);
  HidComposite.SetReportParser(1, &MousePrs);
  HidKeyboard.SetReportParser(0, &keyboardPrs);
  HidMouse.SetReportParser(0, &MousePrs);

  claerKeyEntry();
//  MsTimer2::set(REP_INTERVAL, sendRepeat); 
//  MsTimer2::start();
  rep_timer(); 

//  Serial.println(F("Start."));

}



void loop() {

  Usb.Task();
#if MYDEBUG == 1  
  Serial.println(Usb.getUsbTaskState());
  if ( (classType == 0) && (Usb.getUsbTaskState() == USB_STATE_RUNNING) )  {  
    // デバイスクラス情報の取得
    getIntClass(classType, subClassType) ;
    Serial.print(F("class="));  Serial.println(classType, HEX);
    Serial.print(F("subclass="));  Serial.println(subClassType, HEX);
  }

  static uint8_t prevSts = 0xFF;
  if (Usb.getUsbTaskState() != prevSts) {
    prevSts = Usb.getUsbTaskState();
    Serial.print(F("sts="));  Serial.println(prevSts, HEX);
  }
  if (Serial.available()) {
    Serial.read();
    for (uint8_t i=-0; i < MAXKEYENTRY; i++) {
      Serial.print(F("keyentry["));
      Serial.print(i,DEC);
      Serial.print(F("]="));
      Serial.println(keyentry[i],HEX);
    }
  }
#endif

//
// マウスデータ送信部分
//
  KBDSerial.listen();
  if (KBDSerial.available()) {  //データなしは-1が流れてる
    MSCTRL = KBDSerial.read();
    if (MSCTRL == 64 && oldCTRL == 65) {  //highからlowになった
#if MYDEBUG == 2
      Serial.print(F("MSCTRL = ")); Serial.print(MSCTRL);
      Serial.print(F(" LEFT  = ")); Serial.print(LeftButton); Serial.print(F(" RIGHT = ")); Serial.print(RightButton);
      Serial.print(F(" dX = ")); Serial.print(dx,HEX); Serial.print(F(" 2dX = ")); Serial.print(dx,BIN);
      Serial.print(F(" dX = ")); Serial.print(dx>>1,HEX); Serial.print(F(" 2dX = ")); Serial.print(dx>>1,BIN);
      Serial.print(F(" dY = ")); Serial.print(dy,HEX); Serial.print(F(" 2dY = ")); Serial.print(dy,BIN);
      Serial.print(F(" dY = ")); Serial.print(dy>>1,HEX); Serial.print(F(" 2dY = ")); Serial.println(dy>>1,BIN);
#endif

//    delay抜いても何故か動いてます
//      delayMicroseconds(700);
      MSDATA = B00000000;
      if (LeftButton) MSDATA |= B00000001;    // 左クリック
      if (RightButton) MSDATA |= B00000010;   // 右クリック
//      Serial.print(F("MSDATA = ")); Serial.println(MSDATA);
      Serial.write(MSDATA);
      Serial.write(dx);
      dx=0; // 一度送信したらリセット
      Serial.write(dy);
      dy=0; // 一度送信したらリセット
    }
    if (MSCTRL == 64 || MSCTRL == 65) {
      oldCTRL = MSCTRL;   //MSCTRLデータのみ保存（他のデータも流れてくるかもしれないので）
    }

    // キーリピート開始時間（未実装）
    if (MSCTRL>>4 == 0x06) {
//      Serial.print("DELAY : ");
//      Serial.println(200+(MSCTRL&B00001111)*100);
    }
    // キーリピート間隔（実機と異なる）
    if (MSCTRL>>4 == 0x07) {
//      Serial.print("REP   : ");
//      Serial.println(30+(MSCTRL&B00001111)*(MSCTRL&B00001111)*5);
      REP_INTERVAL = 30+(MSCTRL&B00001111)*(MSCTRL&B00001111)*5;
      rep_timer();
    }
  }
}
