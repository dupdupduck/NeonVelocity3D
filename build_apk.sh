#!/bin/bash
set -e

PROJECT_DIR="/root/NeonVelocity3D"
cd "${PROJECT_DIR}"

ANDROID_JAR="/data/data/com.termux/files/home/.android-sdk/android.jar"
KHRONOS_INC="/root/include"

echo "=========================================================="
echo "    REBUILDING NEON VELOCITY 3D (FIXING CRASH ISSUES)     "
echo "=========================================================="

mkdir -p build/classes
mkdir -p lib/arm64-v8a
rm -f build/app.unaligned.apk build/app.aligned.apk build/NeonVelocity3D.apk

echo "[1/8] Setting up system library stubs..."
mkdir -p /root/syslibs
ln -sf /system/lib64/libEGL.so /root/syslibs/libEGL.so
ln -sf /system/lib64/libGLESv2.so /root/syslibs/libGLESv2.so
ln -sf /system/lib64/libandroid.so /root/syslibs/libandroid.so
ln -sf /system/lib64/liblog.so /root/syslibs/liblog.so

echo "[2/8] Bundling libc++_shared.so for ARM64 Android..."
cp -f /data/data/com.termux/files/usr/lib/libc++_shared.so lib/arm64-v8a/libc++_shared.so
chmod 755 lib/arm64-v8a/libc++_shared.so

echo "[3/8] Compiling Java MainActivity (ecj)..."
ecj -d build/classes -cp "${ANDROID_JAR}" src/java/com/dupdupduck/neonvelocity/MainActivity.java

echo "[4/8] Generating classes.dex (dx)..."
dx --dex --output=build/classes.dex build/classes

echo "[5/8] Compiling Native C++ Game Engine with System Linkage..."
clang++ -shared -fPIC -std=c++17 -O3 \
    -I"${KHRONOS_INC}" \
    -L/root/syslibs \
    -L/data/data/com.termux/files/usr/lib \
    src/cpp/main.cpp \
    -o lib/arm64-v8a/libneonvelocity.so \
    -landroid -llog -lEGL -lGLESv2

echo "Verifying dynamic dependencies of libneonvelocity.so:"
readelf -d lib/arm64-v8a/libneonvelocity.so | grep NEEDED

echo "[6/8] Packaging Manifest & Resources with aapt..."
aapt package -f \
    -M AndroidManifest.xml \
    -S res \
    -I "${ANDROID_JAR}" \
    -F build/app.unaligned.apk

echo "[7/8] Adding DEX and Native Libraries to APK..."
cd build && aapt add app.unaligned.apk classes.dex
cd "${PROJECT_DIR}" && zip -0 -u build/app.unaligned.apk lib/arm64-v8a/libc++_shared.so lib/arm64-v8a/libneonvelocity.so

echo "[8/8] 4KB Page Aligning & Signing APK..."
zipalign -f -p 4 build/app.unaligned.apk build/app.aligned.apk

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
