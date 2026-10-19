#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
#  Trackly Desktop — публикация репозитория на GitHub (аккаунт jaraslaukunin)
#
#  Использование:
#      gh auth login                       # один раз
#      ./scripts/publish-to-github.sh
#
#  Переопределяется через окружение:
#      OWNER=jaraslaukunin REPO_NAME=Trackly VISIBILITY=public \
#          ./scripts/publish-to-github.sh
#
#  Скрипт НЕ создаёт теги и релизы — только репозиторий и ветку main.
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

OWNER="${OWNER:-jaraslaukunin}"
REPO_NAME="${REPO_NAME:-Trackly}"
VISIBILITY="${VISIBILITY:-public}"
DESCRIPTION="${DESCRIPTION:-Дневник эпилепсии и контроля терапии — Qt 6 + Supabase (учебный проект)}"

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

if ! command -v gh >/dev/null 2>&1; then
    echo "Не найден GitHub CLI. Установи: https://cli.github.com" >&2
    exit 1
fi

if ! gh auth status >/dev/null 2>&1; then
    echo "Нужна авторизация. Выполни: gh auth login" >&2
    exit 1
fi

BRANCH="$(git branch --show-current)"
if [ "$BRANCH" != "main" ]; then
    echo "Публикуем только ветку main (сейчас активна: $BRANCH)." >&2
    exit 1
fi

if [ -n "$(git status --porcelain)" ]; then
    echo "Рабочее дерево не чистое — сначала закоммить или спрячь изменения." >&2
    exit 1
fi

if gh repo view "$OWNER/$REPO_NAME" >/dev/null 2>&1; then
    echo "Репозиторий $OWNER/$REPO_NAME уже существует — пушу ветку main."
    git remote get-url origin >/dev/null 2>&1 || git remote add origin "git@github.com:$OWNER/$REPO_NAME.git"
    git push -u origin main
else
    gh repo create "$OWNER/$REPO_NAME" "--$VISIBILITY" \
        --source=. --remote=origin --push \
        --description "$DESCRIPTION"
fi

echo
echo "Готово: https://github.com/$OWNER/$REPO_NAME"
echo "Напоминание: релизы и теги не публикуем."
