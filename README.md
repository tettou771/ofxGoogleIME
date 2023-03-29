# ofxGoogleIME

![Thumbnail image](thumbnail.png)

これは作りかけのプロジェクトです。
NotoSansJPをインストールしたPCで、googleのIMEをWebAPI経由で利用して文字入力するアドオンです。
HTTPリクエストを使って変換候補を毎回聞くというかなり無理やりな実装ですが、意外に遅延が少ないので一応使えそうです。
作りかけなので、使い方などはコロコロ変わると思います。
OSのIMEを切らないと、キーバインドがバッティングする可能性はあります。

# Usage

入力方法の切り替えは、Alt + ` （バッククォート）です。
インターネット接続が必要です。
exampleを実行すると、挙動が確認できます。

ofApp.h

```cpp
ofxGoogleIME ime;
```

ofApp.cpp

```cpp
// setup
float fontSize = 20;
ime.setup("フォント名", fontSize);
```


# Tested system

Mac + of0.11.2

# Author

http://github.com/tettou771
