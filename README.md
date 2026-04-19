# keyremapper

macOS上でキーリマップを行うCLIデーモン。  
CGOを用いてmacOSネイティブAPIを直接呼び出す。

## 機能

- 任意のキーを別のキーにリマップ（修飾キーの組み合わせも可）
- Left/Right Command 単押しでIME切り替え（英数/かな）
- キーボードを VendorID / ProductID で識別し、デバイスごとに設定を切り替え
- ログイン時自動起動・クラッシュ時自動再起動（launchd）

## 設定例（Windowsキーボード向け）

| 物理キー | リマップ先 |
|---------|-----------|
| Left/Right Alt | Left/Right Command |
| Win キー | Left Option |
| Application キー | Right Option |
| Home キー | Cmd+← （行頭移動） |
| End キー | Cmd+→ （行末移動） |

## 動作確認環境

- macOS 26 / Apple Silicon (arm64)

## インストール

Apple Developer証明書による署名・公証を行っていないため、バイナリは配布していません。ソースからビルドしてください。

```bash
go build -o keyremapper ./cmd/keyremapper/
cd install && ./install.sh
```

インストール後、**システム設定 → プライバシーとセキュリティ → アクセシビリティ** で `keyremapper` を手動で許可する必要があります。

## 設定

`~/.config/keyremapper/settings.json` でキーマップをカスタマイズできます。

```json
{
  "common": {
    "ime_switching": true,
    "keymap": []
  },
  "profiles": [
    {
      "name": "My Windows Keyboard",
      "vendor_id": "0x045E",
      "product_id": "0x07A5",
      "ime_switching": true,
      "keymap": [
        { "from": 58,  "to": 55 },
        { "from": 61,  "to": 54 },
        { "from": 55,  "to": 58 },
        { "from": 110, "to": 61 },
        { "from": 115, "to": 123, "modifiers": ["command"] },
        { "from": 119, "to": 124, "modifiers": ["command"] }
      ]
    }
  ]
}
```

**プロファイル解決の優先順位**: VendorID+ProductID一致 → `common` → ハードウェアデフォルト（何もしない）

接続中のキーボードの VendorID / ProductID は以下で確認できます：

```bash
keyremapper --list-devices
```

## CLI

```bash
keyremapper                    # デーモン起動
keyremapper --list-devices     # 認識中のキーボードを表示
keyremapper --config <path>    # 設定ファイルのパスを指定
```

## ビルド

CGOを使用するため、**macOS上でのみビルド可能**です。

```bash
go build -o keyremapper ./cmd/keyremapper/
```

ユニバーサルバイナリ（Apple Silicon + Intel 両対応）:

```bash
GOARCH=arm64 go build -o keyremapper-arm64 ./cmd/keyremapper/
GOARCH=amd64 go build -o keyremapper-amd64 ./cmd/keyremapper/
lipo -create -output keyremapper keyremapper-arm64 keyremapper-amd64
```

## 注意事項

- アクセシビリティ権限はユーザーによる手動許可が必須（自動化不可）
- バイナリのパスが変わるとアクセシビリティ権限が無効になり、再登録が必要
- 設定ファイルが存在しない場合はハードウェアデフォルト（何もしない）で動作する
