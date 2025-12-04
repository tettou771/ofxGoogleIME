# ofxGoogleIME

![Thumbnail image](thumbnail.png)

GoogleのIME WebAPIを利用して日本語入力するopenFrameworksアドオンです。

HTTPリクエストを使って変換候補を取得する実装ですが、非同期処理により描画をブロックしません。

macOSではOS側のIME状態と自動的に同期します。

# Usage

インターネット接続が必要です。
exampleを実行すると、挙動が確認できます。

## 入力切り替え
OS側のIME切り替えに完全連動します（macOS / Windows両対応）。

ofApp.h

```cpp
ofxGoogleIME ime;
```

ofApp.cpp

```cpp
// setup
ime.setFont("フォント名", 20);
ime.enable();

// draw
ime.draw(20, 60);  // 任意の座標で描画
```

# Tested system

Mac + of0.12.1

# Author

http://github.com/tettou771
