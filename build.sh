#!/bin/sh
# Build NBlood (Blood, Redneck Rampage, PowerSlave) for the MiSTer in the
# toolchain container: static SDL2 with the hybrid core drivers first (once),
# then the games.
# usage: ./build.sh [mister|host] [Release|RelWithDebInfo|Debug]
#   mister  ARM binaries for the MiSTer   -> build/mister/
#   host    same code for this PC, to run against hybrid/tools/fakecore
set -e
cd "$(dirname "$0")"
TARGET=${1:-mister}
TYPE=${2:-Release}
ROOT=$PWD
PREFIX=$ROOT/build/sdl/$TARGET/prefix

hybrid/docker.sh hybrid/sdl2/build.sh "$TARGET" build/sdl

# GAME: NBlood has no CMake build: EDuke32's makefiles, with MISTER_HYBRID=1
# (nblood/Common.mak: the classic renderer alone, no network, no OpenGL) and
# our SDL2. Three programs, made in nblood/ and moved to build/$TARGET/:
# nblood (Blood), rednukem (Redneck Rampage), pcexhumed (PowerSlave). NBlood
# (src/nblood.c) is what the launcher starts: it picks the game and becomes
# the program that runs it. nblood is called nblood-bin here: the MiSTer's
# SD card (exFAT) does not tell NBlood and nblood apart, nor blood and the
# data folder Blood/ next to it.
MAKEOPTS="MISTER_HYBRID=1 PRETTY_OUTPUT=0 LTO=0 obj=$ROOT/build/$TARGET/obj SDLCONFIG=$PREFIX/bin/sdl2-config"
case $TYPE in
    Release) MAKEOPTS="$MAKEOPTS RELEASE=1" ;;
    RelWithDebInfo) MAKEOPTS="$MAKEOPTS RELEASE=1 FORCEDEBUG=1 STRIP=" ;;
    *) MAKEOPTS="$MAKEOPTS RELEASE=0 STRIP=" ;;
esac
# MISTER_PROFILE=1 ./build.sh: the profiling build (the release's code with
# its symbols; nblood/source/build/include/mister.h), for development only.
# Remove build/$TARGET/obj when switching: objects of other flags do not link.
[ -n "$MISTER_PROFILE" ] && MAKEOPTS="$MAKEOPTS MISTER_PROFILE=$MISTER_PROFILE STRIP="
CC=gcc
ARCH=
if [ "$TARGET" = mister ]; then
    # the compiler's runtime is linked in, the binary only needs the MiSTer's glibc
    MAKEOPTS="$MAKEOPTS CROSS=arm-linux-gnueabihf- ARCH=armv7l"
    MAKEOPTS="$MAKEOPTS OPTOPT='-mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -fno-pie'"
    MAKEOPTS="$MAKEOPTS CUSTOMOPT='-static-libstdc++ -static-libgcc'"
    CC=arm-linux-gnueabihf-gcc
    ARCH="-mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -static"
fi
mkdir -p "build/$TARGET"
PROGRAMS=${PROGRAMS:-"blood rr exhumed"}
hybrid/docker.sh sh -c "make -C nblood -j\$(nproc) $PROGRAMS $MAKEOPTS"
[ -f nblood/nblood ] && mv -f nblood/nblood "build/$TARGET/nblood-bin"
for p in rednukem pcexhumed; do
    [ -f "nblood/$p" ] && mv -f "nblood/$p" "build/$TARGET/$p"
done
hybrid/docker.sh $CC -O2 $ARCH -Wall -Ihybrid/hps -o "build/$TARGET/NBlood" src/nblood.c \
    hybrid/hps/*.c -lpthread -lm
ls -la build/$TARGET/nblood-bin build/$TARGET/rednukem build/$TARGET/pcexhumed build/$TARGET/NBlood

# The launch script's messages on the core's picture (hybrid/tools/mh_message.c)
if [ "$TARGET" = mister ]; then
    hybrid/docker.sh arm-linux-gnueabihf-gcc -O2 -static -Ihybrid/hps -o build/mister/mh_message \
        hybrid/tools/mh_message.c hybrid/hps/*.c -lpthread -lm
fi
