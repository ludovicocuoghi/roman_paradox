#!/bin/bash

APP_NAME="Rome Interdimensional Journey"
APP_DIR="${APP_NAME}.app"
CONTENTS_DIR="${APP_DIR}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"
FRAMEWORKS_DIR="${CONTENTS_DIR}/Frameworks"

# Create app bundle structure
mkdir -p "${MACOS_DIR}"
mkdir -p "${RESOURCES_DIR}"
mkdir -p "${FRAMEWORKS_DIR}"

# Copy executable
cp bin/sfml_app "${MACOS_DIR}/${APP_NAME}"

# Copy resources (maintaining structure)
cp -r bin/assets "${RESOURCES_DIR}/"
cp -r bin/fonts "${RESOURCES_DIR}/"
cp -r bin/images "${RESOURCES_DIR}/"
cp -r bin/levels "${RESOURCES_DIR}/"

# Copy SFML frameworks (if needed)
cp -R /opt/homebrew/opt/sfml@2/lib/*.dylib "${FRAMEWORKS_DIR}/"

# Fix library paths
install_name_tool -change /opt/homebrew/opt/sfml@2/lib/libsfml-graphics.2.5.dylib @executable_path/../Frameworks/libsfml-graphics.2.5.dylib "${MACOS_DIR}/${APP_NAME}"
install_name_tool -change /opt/homebrew/opt/sfml@2/lib/libsfml-window.2.5.dylib @executable_path/../Frameworks/libsfml-window.2.5.dylib "${MACOS_DIR}/${APP_NAME}"
install_name_tool -change /opt/homebrew/opt/sfml@2/lib/libsfml-system.2.5.dylib @executable_path/../Frameworks/libsfml-system.2.5.dylib "${MACOS_DIR}/${APP_NAME}"

# Create Info.plist
cat > "${CONTENTS_DIR}/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>${APP_NAME}</string>
    <key>CFBundleIdentifier</key>
    <string>com.yourcompany.${APP_NAME}</string>
    <key>CFBundleName</key>
    <string>${APP_NAME}</string>
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

# Set proper permissions
chmod +x "${MACOS_DIR}/${APP_NAME}"

echo "App bundle created at ${APP_DIR}"
