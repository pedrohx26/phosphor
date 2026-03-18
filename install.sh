#!/bin/bash
# Install the compiled Phosphor KWin effect plugin

set -e  # Exit on error

echo "Installing Phosphor KWin effect plugin..."

# Deploy compiled plugin
sudo cp /home/pieter/Code/phosphor/src/build/kwin_effect_retro_term.so \
    /usr/lib/qt6/plugins/kwin/effects/plugins/

# Create effects data directory  
sudo mkdir -p /usr/share/kwin/effects/retro-term/

# Deploy shader and metadata
sudo cp /home/pieter/Code/phosphor/src/retro.frag \
    /usr/share/kwin/effects/retro-term/
sudo cp /home/pieter/Code/phosphor/src/metadata.json \
    /usr/share/kwin/effects/retro-term/

# Reload KWin
echo "Notifying KWin of plugin installation..."
qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null || echo "(KWin reload may require manual restart)"

echo "✓ Installation complete!"
echo "  Plugin: /usr/lib/qt6/plugins/kwin/effects/plugins/kwin_effect_retro_term.so"
echo "  Data:   /usr/share/kwin/effects/retro-term/"
echo ""
echo "To enable the effect:"
echo "  System Settings → Workspace Behavior → Screen Effects → 'Phosphor CRT'"
