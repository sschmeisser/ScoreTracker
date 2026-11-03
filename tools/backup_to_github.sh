#!/usr/bin/env bash
# ScoreTracker automated GitHub backup script
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_DIR"

echo "=== ScoreTracker GitHub Backup: $(date '+%Y-%m-%d %H:%M:%S') ==="

# Stage all files according to .gitignore
git add -A

# Check if there are staged changes
if git diff --staged --quiet; then
    echo "No changes to commit. Working tree clean."
    exit 0
fi

# Formulate commit message
BRANCH="$(git rev-parse --abbrev-ref HEAD)"
NUM_FILES="$(git diff --staged --name-only | wc -l)"
TIMESTAMP="$(date '+%Y-%m-%d %H:%M:%S')"
COMMIT_MSG="${1:-"Auto-backup: ${TIMESTAMP} (${NUM_FILES} files updated)"}"

echo "Committing ${NUM_FILES} files to branch '${BRANCH}'..."
git commit -m "$COMMIT_MSG"

echo "Pushing to origin/${BRANCH}..."
git push origin "$BRANCH"

echo "=== Backup completed successfully! ==="
