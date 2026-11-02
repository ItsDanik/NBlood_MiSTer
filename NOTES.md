# NBlood for MiSTer: notes

How the port works and how it is built. For playing it, see the [README](README.md).

## Three programs

The NBlood project is three programs on one engine: `nblood` (Blood and Cryptic Passage), `rednukem` (Redneck Rampage and Rides Again) and `pcexhumed` (PowerSlave). `NBlood` (`src/nblood.c`) is what the launcher starts: it picks the game with `MH_PickGame()` and becomes the program that runs it, in that game's folder (`Blood/`, `Redneck/RR/`, `Redneck/RRRA/`, `PowerSlave/`), because the programs' settings files and saved games have the same names. `nblood` is called `nblood-bin` in `build/` and on the SD card: exFAT does not tell `nblood` from `NBlood`.

The core is called NBloodD, not NBlood, because MiSTer Frontier has a core called NBlood, and the line its installer puts in `MiSTer.ini` would take over any core of that name.

## Music

Blood's music is MIDI. With the OSD's *Music* on **MIDI** (the default) it is played by a SoundFont 2 synthesizer in the FPGA core, with GeneralUser GS, which comes with the port, or a SoundFont of your own. With *Music* on **FM chip**, it plays as an AdLib or Sound Blaster card did, on a YM3812 (OPL2) in the FPGA core, [JTOPL](https://github.com/jotego/jtopl).

**With an [MT32-pi](https://github.com/dwhinham/mt32-pi)** on the MiSTer's user port and *Music* on MIDI, the music goes to it instead of the synthesizer. The OSD then shows its lines: *MT32-pi* on or off, its synth and ROM, its SoundFont (*Pi's own* keeps the one the Pi has chosen) and *Reset Hanging Notes*.

## Building

Everything builds in Docker on a Linux PC.

```sh
git clone --recursive https://github.com/ItsDanik/NBlood_MiSTer.git
cd NBlood_MiSTer
./build.sh              # ARM game binaries -> build/mister/
./core/build_core.sh    # FPGA core (Quartus Lite 17.0.2) -> core/output_files/NBlood.rbf
./package.sh            # release zip      -> dist/NBloodD_YYYYMMDD.zip
```

The toolchain image (Debian bullseye, glibc 2.31 to match the MiSTer) is built from `hybrid/toolchain/` on first use. The first build also downloads and builds SDL2, SDL2_mixer and SDL2_net.

### Repository layout

| Path | |
|---|---|
| `hybrid/` | submodule: [ItsDanik/Hybrid_MiSTer](https://github.com/ItsDanik/Hybrid_MiSTer), what all our hybrid ports share: the FPGA host module, the ARM side library, SDL2 drivers, the launcher (`danik_hybrid_ports.sh`), toolchain and conventions. See its README. |
| `core/` | FPGA core, based on [Template_MiSTer](https://github.com/MiSTer-devel/Template_MiSTer): `NBlood.sv` (OSD and button names) around `hybrid/rtl/hybrid_host.sv` |
| `nblood/` | submodule: the game, branch `mister`, with the MiSTer changes |
| `package/` | files shipped in the release next to the binary (`danik_hybrid_launch.sh`, README), and `games.txt`: the games and their menu entries |
| `releases/` | release packages |

### Development notes

- `./build.sh host` builds the same game for the PC. It runs against `hybrid/tools/fakecore`, a stand-in for the FPGA core that takes scripted input and saves screenshots and audio:
  ```sh
  gcc -O2 -o build/host/fakecore hybrid/tools/fakecore.c
  build/host/fakecore /tmp/shm script.txt audio.raw &
  cd <data folder> && MISTER_HYBRID_SHM=/tmp/shm <repository>/build/host/nblood
  ```
- `MISTER=root@<address> tools/test_on_mister.sh <id>` tries the build on a MiSTer next to what is installed there, without installing it: from `/tmp`, with the installed game's folder mirrored there (what the game writes stays in `/tmp`) and its own OSD settings. `list` and `nodata` for the core started by itself and without data, `log` for the game's log, `stop` to remove all of it again.
- `touch /tmp/nblood_nolaunch` on the MiSTer keeps the core loaded without starting the game, so you can start a development binary by hand (set `MISTER_HYBRID_CORE=NBloodD`, and `MISTER_HYBRID_SET` for a game's set name). `/tmp/danik_hybrid_ports.log` shows what the launcher daemon did.
