#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BINARY_SRC="$SCRIPT_DIR/../keyremapper"
BINARY_DST="/usr/local/bin/keyremapper"
PLIST_NAME="io.github.wattata.keyremapper.plist"
PLIST_SRC="$SCRIPT_DIR/$PLIST_NAME"
PLIST_DST="$HOME/Library/LaunchAgents/$PLIST_NAME"
CONFIG_DIR="$HOME/.config/keyremapper"
CONFIG_FILE="$CONFIG_DIR/settings.json"

if [ ! -f "$BINARY_SRC" ]; then
    echo "エラー: バイナリが見つかりません: $BINARY_SRC"
    echo "先に 'go build -o keyremapper ./cmd/keyremapper/' でビルドしてください。"
    exit 1
fi

echo "バイナリを配置中..."
sudo cp "$BINARY_SRC" "$BINARY_DST"
sudo chmod 755 "$BINARY_DST"

echo "設定ファイルを配置中..."
mkdir -p "$CONFIG_DIR"
if [ ! -f "$CONFIG_FILE" ]; then
    cat > "$CONFIG_FILE" <<'EOF'
{
  "common": {
    "ime_switching": true,
    "keymap": []
  },
  "profiles": []
}
EOF
    echo "  → $CONFIG_FILE を作成しました"
else
    echo "  → 既存の設定ファイルを保持します: $CONFIG_FILE"
fi

echo "launchd を登録中..."
mkdir -p "$HOME/Library/LaunchAgents"
cp "$PLIST_SRC" "$PLIST_DST"
launchctl unload "$PLIST_DST" 2>/dev/null || true
launchctl load "$PLIST_DST"

echo ""
echo "インストール完了！"
echo ""

echo "【アクセシビリティ権限の確認】"
echo "  バイナリを再ビルドした場合は権限がリセットされます。"
echo "  システム設定 → プライバシーとセキュリティ → アクセシビリティ"
echo "  → keyremapper を一度削除して再追加してください。"
echo ""

echo "動作確認："
echo "  launchctl list | grep keyremapper"
