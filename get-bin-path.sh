#!/bin/sh

ARCH=$(uname -m)
OS=$(uname -s)

if [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then
	ARCH="arm64"
fi
if [ "$OS" = "Linux" ] || [ "$OS" = "GNU/Linux" ]; then
	OS="linux"
fi
if [ "$OS" = "Darwin" ]; then
	OS="macos"
fi

echo "$ARCH/bin/$OS"
