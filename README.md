# MGSVFix
[![Patreon-Button](.github/images/Patreon-Button.png)](https://www.patreon.com/Wintermance) [![ko-fi](.github/images/Kofi-Button.svg)](https://ko-fi.com/W7W01UAI9)

**MGSVFix** is an ASI plugin for *Metal Gear Solid V: The Phantom Pain* (and *Ground Zeroes*!) that can:
- Unlock resolution options/support.
- Unlock framerate.
- Fix HUD issues at ultrawide resolutions.
- Fix graphical effects at ultrawide resolutions.

For more details on exactly what is fixed, click [here](./fixes.md).

## Installation  
- Download the latest [release](../../../releases). 
- Extract the contents of the release zip in to the the game folder.  

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**  
- Open the game properties in Steam and add `WINEDLLOVERRIDES="winmm=n,b" %command%` to the launch options.  

## Configuration
- Open **`MGSVFix.ini`** to adjust settings.

## Known Issues
#### Ultrawide
- Certain lens effects like lens dirt are displayed at 16:9.

## Screenshots
| ![ezgif-4404660a5886d0](https://github.com/user-attachments/assets/58f40f24-1306-49e7-b4cd-1b90feacd2f3) |
|:--:|
| Gameplay |

## Credits
Thanks to Hotiraripha for commissioning this fix! <br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
