# LinuxLabX

Beginner-friendly Linux learning desktop app.

[![Download for macOS](https://img.shields.io/badge/Download-macOS-black?style=for-the-badge&logo=apple)](https://github.com/Iankitsinghak/LinuxLabX/releases/latest/download/LinuxLabX-mac.dmg)
[![Download for Windows](https://img.shields.io/badge/Download-Windows-0078D6?style=for-the-badge&logo=windows)](https://github.com/Iankitsinghak/LinuxLabX/releases/latest/download/LinuxLabX-win.exe)
[![Download for Linux](https://img.shields.io/badge/Download-Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)](https://github.com/Iankitsinghak/LinuxLabX/releases/latest/download/LinuxLabX-linux.AppImage)

## Screenshots

![LinuxLabX Main Window](assets/screenshots/main-window.png)
![LinuxLabX Tutorial](assets/screenshots/tutorial.png)
![LinuxLabX Help](assets/screenshots/help.png)

## Features

- No setup required
- Interactive Linux learning
- Cross-platform support

## Installation

### macOS (DMG)

1. Download [LinuxLabX-mac.dmg](https://github.com/Iankitsinghak/LinuxLabX/releases/latest/download/LinuxLabX-mac.dmg)
2. Open the DMG
3. Drag LinuxLabX into Applications

### Windows (EXE)

1. Download [LinuxLabX-win.exe](https://github.com/Iankitsinghak/LinuxLabX/releases/latest/download/LinuxLabX-win.exe)
2. Run installer
3. Launch LinuxLabX from Start Menu

### Linux (AppImage)

1. Download [LinuxLabX-linux.AppImage](https://github.com/Iankitsinghak/LinuxLabX/releases/latest/download/LinuxLabX-linux.AppImage)
2. Make executable:
	```bash
	chmod +x LinuxLabX-linux.AppImage
	```
3. Run:
	```bash
	./LinuxLabX-linux.AppImage
	```

## Quick Start

1. Launch LinuxLabX
2. Choose Start Tutorial on first run
3. Try your first command:
	```text
	ls
	```
4. Continue with:
	```text
	help
	tutorial
	```

## Troubleshooting

### App does not start

- Re-download latest installer from Releases
- Check that your OS is supported
- Open logs from crash dialog, or check local app data logs

### Linux AppImage does not run

- Ensure executable permission is set
- Install FUSE package if your distro requires it

### Update notification not visible

- Ensure internet connectivity
- Verify GitHub API access is not blocked by firewall/proxy

## Contributing

Contributions are welcome.

1. Fork repository
2. Create branch: `feature/my-change`
3. Commit and push
4. Open a Pull Request
