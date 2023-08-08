# Source2 Swapping
Swapping files loaded by Source 2

## Information
Ideologically this project is similar to [the implementation for CS:GO](https://github.com/AspectUnk/panorama-swapping/) (Source 1), but can be used for almost all games on Source 2 and replaces absolutely all files (in VPK and more), as panoramas as well as any others.

## Using
- To swap a file you must copy it to the `game\bin\win64\swapping` directory (if there is no swapping folder, create one) with the hierarchy intact. File hierarchy in VPK can be viewed using [VRF](https://github.com/SteamDatabase/ValveResourceFormat).
- For a file to be successfully swapped, the library must be loaded before the file itself is loaded by the game. The best way to do this is to use the Xenos Injector in Manual Launch mode
- Inject the library only with `-insecure` command line for account security

## Credits
A little about other:

- **[zephire](https://github.com/zephire1)** - kicked me for realization

This project uses the following libraries:

- **[fmtlib](https://github.com/fmtlib/fmt)** - formatting library
- **[minhook](https://github.com/TsudaKageyu/minhook)** - hooking library
