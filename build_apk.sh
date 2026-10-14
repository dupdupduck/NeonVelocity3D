#!/bin/bash
set -e

PROJECT_DIR="/root/NeonVelocity3D"
cd "${PROJECT_DIR}"

ANDROID_JAR="/data/data/com.termux/files/home/.android-sdk/android.jar"
KHRONOS_INC="/root/include"

echo "=========================================================="
echo "    BUILDING NEON VELOCITY 3D (C++ OPENGL ES FOR ANDROID) "
echo "=========================================================="

mkdir -p build/classes
mkdir -p lib/arm64-v8a

echo "[1/7] Compiling Java Activity (ecj)..."
ecj -d build/classes -cp "${ANDROID_JAR}" src/java/com/dupdupduck/neonvelocity/MainActivity.java

echo "[2/7] Generating classes.dex (dx)..."
dx --dex --output=build/classes.dex build/classes

echo "[3/7] Compiling Native 3D C++ Engine (clang++ aarch64)..."
clang++ -shared -fPIC -std=c++17 -O3 \
    -I"${KHRONOS_INC}" \
    src/cpp/main.cpp \
    -o lib/arm64-v8a/libneonvelocity.so \
    -landroid -llog -lEGL -lGLESv2

echo "Native binary generated:"
ls -lh lib/arm64-v8a/libneonvelocity.so

echo "[4/7] Packaging Android Resources & Manifest (aapt)..."
aapt package -f \
    -M AndroidManifest.xml \
    -S res \
    -I "${ANDROID_JAR}" \
    -F build/app.unaligned.apk

echo "[5/7] Adding DEX and Native Library to APK (aapt add)..."
cd build && aapt add app.unaligned.apk classes.dex
cd "${PROJECT_DIR}" && aapt add build/app.unaligned.apk lib/arm64-v8a/libneonvelocity.so

echo "[6/7] 4KB Page Aligning APK (zipalign)..."
zipalign -f -p 4 build/app.unaligned.apk build/app.aligned.apk

echo "[7/7] Signing APK with apksigner (v2/v3 scheme)..."
if [ ! -f debug.keystore ]; then
    echo "Creating debug keystore..."
    keytool -genkeypair -v -keystore debug.keystore \
        -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 \
        -storepass android -keypass android \
        -dname "CN=NeonVelocity,O=GameStudio,C=VN"
fi

apksigner sign --ks debug.keystore \
    --ks-pass pass:android --key-pass pass:android \
    --ks-key-alias androiddebugkey \
    --out build/NeonVelocity3D.apk \
    build/app.aligned.apk

echo "Verifying APK Signature:"
apksigner verify --verbose build/NeonVelocity3D.apk

# Copy directly to Android Download folder for 1-tap installation
if [ -d "/storage/emulated/0/Download" ]; then
    cp -f build/NeonVelocity3D.apk /storage/emulated/0/Download/NeonVelocity3D.apk
    echo "COPIED TO: /storage/emulated/0/Download/NeonVelocity3D.apk"
fi

echo ""
echo "=========================================================="
echo " BUILD SUCCESSFUL! "
echo " Output APK: ${PROJECT_DIR}/build/NeonVelocity3D.apk"
echo " Size: $(ls -lh build/NeonVelocity3D.apk | awk '{print $5}')"
echo "=========================================================="
aapt dump badging build/NeonVelocity3D.apk | grep -E "package|launchable-activity|native-code"
