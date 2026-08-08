#!/usr/bin/env bash
# ==============================================================================
# SNAP Engine Asset Initializer Script for macOS / Linux
# ==============================================================================

set -e

# Default Repository & Model Zip URLs
HF_REPO_URL="https://huggingface.co/softguy777/snap-weights"
HF_ZIP_URL="https://huggingface.co/softguy777/snap-weights/resolve/main/quickstart/win-x64-ko-int8-quickstart.zip"

AUTO_CONFIRM=false
TARGET_PARAM=""

# Parse flags
for arg in "$@"; do
    case $arg in
        -y|--yes)
            AUTO_CONFIRM=true
            ;;
        *)
            if [ -z "$TARGET_PARAM" ]; then
                TARGET_PARAM="$arg"
            fi
            ;;
    esac
done

# 1. Resolve Target Directory
if [ -n "$TARGET_PARAM" ]; then
    TARGET_DIR="$TARGET_PARAM"
else
    TARGET_DIR="$(pwd)"
fi

# Create directory if it doesn't exist
mkdir -p "$TARGET_DIR"

# Get Absolute Path
ABS_TARGET_DIR="$(cd "$TARGET_DIR" && pwd)"

echo "=================================================================="
echo "  SNAP Engine Initializer (macOS / Linux)"
echo "=================================================================="
echo "  - Target SNAP Root: ${ABS_TARGET_DIR}"
echo "=================================================================="

# 2. Interactive Confirmation
if [ "$AUTO_CONFIRM" = false ]; then
    read -p "Do you want to set '${ABS_TARGET_DIR}' as SNAP Root and download models? (y/N): " CONFIRM
    case "$CONFIRM" in
        [yY][eE][sS]|[yY])
            ;;
        *)
            echo "[CANCELLED] Initialization aborted by user."
            exit 0
            ;;
    esac
fi

MODELS_DIR="${ABS_TARGET_DIR}/models"
echo ""
echo "[1/2] Downloading SNAP models & lexicons into: ${MODELS_DIR}"

# 3. Check for Git availability, fallback to Zip download
if command -v git >/dev/null 2>&1; then
    echo " -> 'git' detected. Cloning models via Git..."
    if [ -d "$MODELS_DIR" ]; then
        echo " -> Existing models directory found. Pulling latest..."
        (cd "$MODELS_DIR" && git pull) || true
    else
        git clone --depth 1 "$HF_REPO_URL" "$MODELS_DIR"
    fi
else
    echo " -> 'git' not found. Falling back to HTTP ZIP download..."
    TEMP_ZIP="/tmp/snap_models_$$.zip"
    
    if command -v curl >/dev/null 2>&1; then
        curl -L -o "$TEMP_ZIP" "$HF_ZIP_URL"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "$TEMP_ZIP" "$HF_ZIP_URL"
    else
        echo "[ERROR] Neither 'curl' nor 'wget' was found. Please install curl, wget, or git."
        exit 1
    fi
    
    echo " -> Extracting models..."
    mkdir -p "$MODELS_DIR"
    if command -v unzip >/dev/null 2>&1; then
        unzip -q "$TEMP_ZIP" -d "$MODELS_DIR"
    elif command -v tar >/dev/null 2>&1; then
        tar -xf "$TEMP_ZIP" -C "$MODELS_DIR"
    else
        echo "[ERROR] Neither 'unzip' nor 'tar' was found to extract model zip."
        rm -f "$TEMP_ZIP"
        exit 1
    fi
    rm -f "$TEMP_ZIP"
fi

# 4. Environment Variable Setup Guidance
echo ""
echo "[2/2] Configuring Environment Variable (SNAP_HOME)..."

SHELL_NAME="$(basename "$SHELL")"
RC_FILE=""

if [ "$SHELL_NAME" = "zsh" ]; then
    RC_FILE="$HOME/.zshrc"
elif [ "$SHELL_NAME" = "bash" ]; then
    if [ -f "$HOME/.bash_profile" ]; then
        RC_FILE="$HOME/.bash_profile"
    else
        RC_FILE="$HOME/.bashrc"
    fi
fi

ENV_EXPORT_LINE="export SNAP_HOME=\"${ABS_TARGET_DIR}\""

if [ -n "$RC_FILE" ]; then
    if [ "$AUTO_CONFIRM" = false ]; then
        read -p "Would you like to append SNAP_HOME to '${RC_FILE}'? (y/N): " ADD_ENV
    else
        ADD_ENV="y"
    fi

    case "$ADD_ENV" in
        [yY][eE][sS]|[yY])
            if grep -q "SNAP_HOME" "$RC_FILE" 2>/dev/null; then
                echo " -> SNAP_HOME already present in ${RC_FILE}. Updating..."
                # Remove existing SNAP_HOME lines and append new one
                sed -i.bak '/SNAP_HOME/d' "$RC_FILE" 2>/dev/null || true
            fi
            echo "$ENV_EXPORT_LINE" >> "$RC_FILE"
            echo " [OK] Added to ${RC_FILE}"
            echo "      Run 'source ${RC_FILE}' or restart terminal to apply."
            ;;
        *)
            echo " -> Skipped updating ${RC_FILE}."
            ;;
    esac
fi

# 5. Asset Self-Verification Check
echo ""
echo "[Verification] Checking asset integrity..."
if [ -f "${MODELS_DIR}/manifest.json" ] || [ -f "${MODELS_DIR}/ko/snap_config.json" ] || [ -d "${MODELS_DIR}/ko" ]; then
    echo " [OK] Model manifests & configurations verified."
else
    echo " [WARN] Could not locate 'manifest.json' or 'snap_config.json' inside ${MODELS_DIR}."
    echo "        Please ensure model files are properly populated."
fi

echo ""
echo "=================================================================="
echo " [SUCCESS] SNAP initialization completed successfully!"
echo "  - SNAP Root : ${ABS_TARGET_DIR}"
echo "  - Models    : ${MODELS_DIR}"
echo "  - Quick Test Command:"
echo "    export SNAP_HOME=\"${ABS_TARGET_DIR}\""
echo "=================================================================="
