#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════════
# Trackly Desktop — сборка дистрибутива для macOS (.dmg)
# ═══════════════════════════════════════════════════════════════════════════════
#
# Использование:
#   chmod +x scripts/package.sh
#   ./scripts/package.sh
#
# Результат: dist/TracklyDesktop-0.1.0.dmg
#
# Требования:
#   - Qt 6 (с macdeployqt в PATH, либо задать QT_BIN_DIR)
#   - CMake 3.21+
#   - Xcode Command Line Tools
# ═══════════════════════════════════════════════════════════════════════════════

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

VERSION="${TRACKLY_VERSION:-0.1.0}"
BUILD_DIR="${BUILD_DIR:-build/release}"
STAGE_DIR="${STAGE_DIR:-dist/stage}"
DMG_NAME="TracklyDesktop-${VERSION}.dmg"
DMG_DIR="${DMG_DIR:-dist}"

echo "════════════════════════════════════════════════════════════════════"
echo "  Trackly Desktop — macOS package builder"
echo "  Version:  ${VERSION}"
echo "  Build:    ${BUILD_DIR}"
echo "  Output:   ${DMG_DIR}/${DMG_NAME}"
echo "════════════════════════════════════════════════════════════════════"

# ─── Шаг 1: Конфигурация ───
echo ""
echo "[1/4] Configuring CMake (Release, TRACKLY_DEPLOY=ON)..."
cmake -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTRACKLY_DEPLOY=ON \
    -G Ninja

# ─── Шаг 2: Сборка ───
echo ""
echo "[2/4] Building..."
cmake --build "${BUILD_DIR}"

# ─── Шаг 3: Установка (install) с macdeployqt ───
echo ""
echo "[3/4] Installing + running macdeployqt..."
rm -rf "${STAGE_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${STAGE_DIR}"

# Проверяем, что фреймворки скопировались
if [ -d "${STAGE_DIR}/Trackly Desktop.app/Contents/Frameworks" ]; then
    FRAMEWORK_COUNT=$(find "${STAGE_DIR}/Trackly Desktop.app/Contents/Frameworks" -maxdepth 1 -name 'Qt*' | wc -l)
    echo "       Frameworks bundled: ${FRAMEWORK_COUNT}"
else
    echo "       WARNING: No Frameworks/ directory found. macdeployqt may have failed."
    echo "       Run with: export QT_BIN_DIR=/path/to/Qt/6.x.x/macos/bin && ./scripts/package.sh"
fi

# ─── Шаг 4: Создание DMG ───
echo ""
echo "[4/4] Creating DMG..."
mkdir -p "${DMG_DIR}"
rm -f "${DMG_DIR}/${DMG_NAME}"

hdiutil create \
    -volname "Trackly Desktop" \
    -srcfolder "${STAGE_DIR}" \
    -ov \
    -format UDZO \
    "${DMG_DIR}/${DMG_NAME}"

echo ""
echo "════════════════════════════════════════════════════════════════════"
echo "  Done: ${DMG_DIR}/${DMG_NAME}"
echo "  Size: $(du -sh "${DMG_DIR}/${DMG_NAME}" | cut -f1)"
echo "════════════════════════════════════════════════════════════════════"
