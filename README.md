# ofxGoogleIME

これは作りかけのプロジェクトです。
NotoSansJPをインストールしたPCで、googleのIMEをWebAPI経由で利用して文字入力するアドオンです。
かなり無理やりな実装なので、あまりお勧めしません。どうしてもOSのIMEを使わずに（あるいは使えない状況で）日本語入力したい場合には、意味があるかもしれません。
作りかけなので、使い方などはコロコロ変わると思います。

# Usage

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

```cpp
// draw
ime.draw()
```



# Tested system

Mac + of0.11.2

# Author

http://github.com/tettou771
