# Fedoraloader

Fedoraloader is an easy-to-use loader for the free and open-source cheat [Fedoraware](https://github.com/Fedoraware/Fedoraware).
It automatically download and injects the latest Fedoraware build into your game.

## Usage

- **Add the loader to you anti virus exceptions!**
- Run Fedoraloader as administrator
- ???
- Profit

## Options

Fedoraloader allows some options to be set via command line arguments.
To use them, create a shortcut to the loader and add the arguments to the target field.

| Argument | Description |
| --- | --- |
| `-silent` | Runs the loader without GUI |
| `-unprotected` | Skips VAC-Bypass and doesn't restart Steam |
| `-debug` | Uses LoadLibrary-Injection (only works with local files) |
| `-file "..."` | Use custom local file (.zip or .dll) |
| `-url "..."` | Custom download URL (.zip or .dll) |

## Credits

- [miniz](https://github.com/richgel999/miniz)
- [VAC Bypass](https://github.com/danielkrupinski/VAC-Bypass)
- [Simple Manual Map Injector](https://github.com/TheCruZ/Simple-Manual-Map-Injector)
