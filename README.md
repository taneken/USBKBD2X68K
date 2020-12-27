# X68000キーボードコネクタにUSBキーボードとUSBマウスを接続します

## おことわり
自分が普段使用しているキーボードやマウスでテストを行っているため、すべてのUSB機器に対して稼働確認が取れているわけではありません。相性や根性が足りない場合動かないものもありますのでご了承ください。

## 必要機材(例)
* Arduino Uno + USB HOSTシールド
* Arduino pro mini(5V) + USB HOSTシールド(pro mini用)

X68000本体から5Vが流れてくるのでArduino側も5Vに合わせる必要があります。
3.3V版arduinoを使用の際はレベルコンバータを使用し5Vにします。

## テスト環境
* ELECOM TK-FDM109MBK(2.4GHz無線キーボード／マウス)
* ELECOM TK-FDM086MBK(2.4GHz無線キーボード／マウス)
* SANWA SUPPLY USB-CVP22(USB-PS/2コンバータケーブル) +
IBM RT3200
* Logicool M171(ワイヤレスマウス)
* ELECOM M-BT12BR(Bluetoothマウス)

## 使えなかったもの
* HHKB Lite2(USBハブを内蔵しているためUSBHOSTのライブラリで具合悪いらしい)

## 配線図
![](keyboard_connector.png)
```
//  本体側             Arduino側
//  -------------------------------------------
//  1:Vcc2 5V(out) -> 5V
//  2:MSDATA(out)  <- TX(1)
//  3:KEYRxD(in)   <- A0(14) softwareSerial TX 
//  4:KEYTxD(out)  -> A1(15) softwareSerial RX
//  5:READY(out)
//  6:REMOTE(in)
//  7:GND(--)      -- GND
```

## PS/2ケーブルの加工
キーボードコネクタは7ピンですが6ピン目のREMOTEはこの変換機では使用していないのでMiniDIN6pケーブルを加工して使用できます。また中央にあるプラスティックの足はニッパーなどで切断することで7ピン端子に刺すことが可能になります。

[カモンのPSK-18](https://www.sengoku.co.jp/mod/sgk_cart/detail.php?code=4AC4-DTEN)なんてお手頃です。1本から2つ採取できます。

![](miniDIN7pin_male.jpg)

## キーアサイン
109キーボードではすべてのキーをアサインすることができません。なので使用頻度の低いキーをアサインしていません。
もし変更したいときはArduinoのソースを修正するか、[KeyWitch.x](http://retropc.net/x68000/software/system/key/keywitch/)等ソフトウェアでキーアサインを変更できるソフトを使用してください。

私はSted2でリズム画面に入るため記号入力や登録キーを押す必要がありますが、Keywitchでsted2を使用するときだけ「BREAK」と「COPY」を「記号入力」と「登録」に入れ替えて使用しています。

![](109.jpg)

#### キーアサイン初期状態
```
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
```

## USB-HUB使用時の挙動（検証中）
`書く`
USBハブに刺した順番に初期化処理を行うようです。キーボードを2つ接続すると先に認識したほうしか入力できない？


## Bluetoothの挙動（検証中）
BluetoothキーボードとBluetoothマウスも使用可能です。


## 実装していないコト
* マウスカーソルのオーバーフロー処理
* マウスの速度調整変更

## 履歴
* 2020.11.09 Ver.0.1	とりあえず動いた版

## 謝意
USBキーボードをX68000に繋げるネタ、情報提供、共同作成したza2さん、ありがとうございます。

技術的情報提供、相談に乗ってもらいましたハルぴこさん、ありがとうございます。

## 父のパソコンを超えろ
