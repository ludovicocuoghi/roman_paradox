#!/bin/bash

# === Configuration ===
APP_NAME="Rome Interstellar Paradox"
APP_BINARY_NAME="rome_interstellar_paradox"
BUILD_DIR="mac_build"
APP_DIR="${BUILD_DIR}/${APP_NAME}.app"
CONTENTS_DIR="${APP_DIR}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"
FRAMEWORKS_DIR="${CONTENTS_DIR}/Frameworks"
SFML_LIB_PATH="/opt/homebrew/opt/sfml@2/lib"

# === Clean previous build ===
rm -rf "${APP_DIR}"
mkdir -p "${MACOS_DIR}" "${RESOURCES_DIR}" "${FRAMEWORKS_DIR}"

# === Copy main binary ===
cp bin/sfml_app "${MACOS_DIR}/${APP_BINARY_NAME}"
chmod +x "${MACOS_DIR}/${APP_BINARY_NAME}"

# === Copy resources ===
cp -r bin/assets "${RESOURCES_DIR}/"
cp -r bin/fonts "${RESOURCES_DIR}/"
cp -r bin/images "${RESOURCES_DIR}/"
cp -r bin/levels "${RESOURCES_DIR}/"

# === Copy icon ===
cp MyIcon.icns "${RESOURCES_DIR}/"

# === Copy only necessary SFML dylibs ===
cp "${SFML_LIB_PATH}/libsfml-graphics"* "${FRAMEWORKS_DIR}/"
cp "${SFML_LIB_PATH}/libsfml-window"* "${FRAMEWORKS_DIR}/"
cp "${SFML_LIB_PATH}/libsfml-system"* "${FRAMEWORKS_DIR}/"

# === Fix dynamic library paths in binary ===
install_name_tool -change "${SFML_LIB_PATH}/libsfml-graphics.2.5.dylib" @executable_path/../Frameworks/libsfml-graphics.2.5.dylib "${MACOS_DIR}/${APP_BINARY_NAME}"
install_name_tool -change "${SFML_LIB_PATH}/libsfml-window.2.5.dylib"   @executable_path/../Frameworks/libsfml-window.2.5.dylib   "${MACOS_DIR}/${APP_BINARY_NAME}"
install_name_tool -change "${SFML_LIB_PATH}/libsfml-system.2.5.dylib"   @executable_path/../Frameworks/libsfml-system.2.5.dylib   "${MACOS_DIR}/${APP_BINARY_NAME}"

# === Create Info.plist ===
cat > "${CONTENTS_DIR}/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>${APP_BINARY_NAME}</string>
    <key>CFBundleIdentifier</key>
    <string>com.yourcompany.rome-interstellar-paradox</string>
    <key>CFBundleName</key>
    <string>${APP_NAME}</string>
    <key>CFBundleIconFile</key>
    <string>MyIcon</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.13</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

# === Clear quarantine flags if needed ===
xattr -dr com.apple.quarantine "${MACOS_DIR}/${APP_BINARY_NAME}" 2>/dev/null
xattr -dr com.apple.quarantine "${APP_DIR}" 2>/dev/null

# === Sign the app (ad-hoc) ===
codesign --force --deep --sign - "${APP_DIR}"

echo "App bundle created at: ${APP_DIR}"
