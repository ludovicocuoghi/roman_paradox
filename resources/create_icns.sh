#!/bin/bash

# This script creates a macOS .icns file from a 128x128 png image

if [ $# -ne 1 ]; then
  echo "Usage: $0 icon.png"
  exit 1
fi

mkdir -p IconBuild.iconset
source_png="$1"

# Check source image dimensions
dimensions=$(sips -g pixelWidth -g pixelHeight "$source_png" | grep -E 'pixel(Width|Height)' | awk '{print $2}')
width=$(echo "$dimensions" | head -n 1)
height=$(echo "$dimensions" | tail -n 1)

if [ "$width" -ne 128 ] || [ "$height" -ne 128 ]; then
  echo "Warning: Your source image is not 128x128. Using it as is but this may affect quality."
  echo "Current dimensions: ${width}x${height}"
fi

# Generate smaller sizes by downscaling
if [ "$width" -ge 16 ] && [ "$height" -ge 16 ]; then
  sips -z 16 16 "$source_png" --out IconBuild.iconset/icon_16x16.png
fi

if [ "$width" -ge 32 ] && [ "$height" -ge 32 ]; then
  sips -z 32 32 "$source_png" --out IconBuild.iconset/icon_32x32.png
  cp IconBuild.iconset/icon_32x32.png IconBuild.iconset/icon_16x16@2x.png
fi

if [ "$width" -ge 64 ] && [ "$height" -ge 64 ]; then
  sips -z 64 64 "$source_png" --out IconBuild.iconset/icon_32x32@2x.png
fi

# Copy the original 128x128 directly
cp "$source_png" IconBuild.iconset/icon_128x128.png

# For larger sizes, we'll duplicate the 128x128 version
# Note: This isn't ideal for quality but will create a valid .icns file
cp "$source_png" IconBuild.iconset/icon_256x256.png
cp "$source_png" IconBuild.iconset/icon_128x128@2x.png
cp "$source_png" IconBuild.iconset/icon_512x512.png 
cp "$source_png" IconBuild.iconset/icon_256x256@2x.png
cp "$source_png" IconBuild.iconset/icon_512x512@2x.png

# Convert to .icns file
iconutil -c icns IconBuild.iconset -o AppIcon.icns

# Clean up
rm -R IconBuild.iconset

echo "Created AppIcon.icns (Warning: Larger sizes were duplicated from 128x128 source)"
echo "For best quality, Apple recommends using a 1024x1024 source image."
