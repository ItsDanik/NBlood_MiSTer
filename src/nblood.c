// NBlood: what the launcher starts. The NBlood project is three programs on
// one engine: nblood (Blood and Cryptic Passage), rednukem (Redneck Rampage
// and Rides Again) and pcexhumed (PowerSlave). This picks the game
// (MH_PickGame(): the menu entry's, or from the list of the games whose files
// are there) and becomes the program that runs it, in that game's folder.
// The game's logo, shown with the pick, goes over to it (MH_Close() before
// execv(), host version 17). Without the game's files the core's picture
// says which go where.
//
// nblood is called nblood-bin next to this program: the SD card (exFAT)
// would not tell nblood from NBlood, nor blood from the data folder Blood/.
//
// Each program has a folder of its own in the current folder
// (games/dnk/NBlood/, DATADIR of danik_hybrid_launch.sh): Blood/ (Cryptic
// Passage with it, as its release and Blood's GOG and Steam versions have
// it), Redneck/ and PowerSlave/. Their settings files and saved games have
// the same names, and Blood's and PowerSlave's data could too. The program
// runs there with -usecwd (else it would move to ~/.config/<program>) and
// finds its .pk3 with -j in the folder of this program (GAMEDIR), where all
// of them are. Redneck Rampage and Rides Again run in a folder of their own
// under Redneck/ (RR/, RRRA/), so that neither lists the other's saved
// games, and find the GRP files with -j; rednukem knows them by their
// checksum, whatever their names (REDNECK.GRP both, or RA.GRP), and starts
// the one DUKE3DGRP names.

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mister_hybrid.h>

#define EXIT_TO_MENU 0

enum { BLOOD, CRYPTIC, RR, RRRA, EXHUMED, NUM_GAMES };

static const char *const BLOOD_FILES[] = {
    "Blood/BLOOD.RFF", "Blood/GUI.RFF", "Blood/SOUNDS.RFF", "Blood/SURFACE.DAT",
    "Blood/TILES0??.ART", "Blood/VOXEL.DAT", NULL };
static const char *const CRYPTIC_FILES[] = {
    "Blood/BLOOD.RFF", "Blood/GUI.RFF", "Blood/SOUNDS.RFF", "Blood/SURFACE.DAT",
    "Blood/TILES0??.ART", "Blood/VOXEL.DAT", "Blood/CRYPTIC.INI", "Blood/CP0?.MAP", NULL };
static const char *const EXHUMED_FILES[] = { "PowerSlave/STUFF.DAT", NULL };

// The ids and titles are those of package/games.txt. The files of the two
// Redneck Rampage games are found by check_redneck().
static const mh_game GAMES[NUM_GAMES] = {
    { "BLOOD", "Blood", BLOOD_FILES, NULL,
      "Blood is not there. Copy everything in your Blood folder (version 1.21: "
      "BLOOD.RFF, GUI.RFF, SOUNDS.RFF, TILES0*.ART among it) into %s/Blood." },
    { "CP", "Blood: Cryptic Passage", CRYPTIC_FILES, NULL,
      "Cryptic Passage is not there. It needs Blood and its own files "
      "(CRYPTIC.INI, CP*.MAP) together: copy both into %s/Blood." },
    { "RR", "Redneck Rampage", NULL, NULL,
      "Redneck Rampage is not there. Copy everything in its folder (REDNECK.GRP "
      "among it) into %s/Redneck." },
    { "RRRA", "Redneck Rampage: Rides Again", NULL, NULL,
      "Redneck Rampage: Rides Again is not there. Copy everything in its folder "
      "(its GRP file, REDNECK.GRP or RA.GRP, among it) into %s/Redneck." },
    { "EX", "PowerSlave (Exhumed)", EXHUMED_FILES, NULL,
      "PowerSlave is not there. Copy everything in the folder of your PowerSlave "
      "or Exhumed for DOS, or of its demo (STUFF.DAT among it), into %s/PowerSlave." },
};

static const char *const NO_GAME =
    "No game found. Copy your games' folders into %s: Blood (with Cryptic "
    "Passage) into Blood, both Redneck Rampage games into Redneck, PowerSlave "
    "into PowerSlave.";

// Each game's program, folder and the argument after its own
static const struct {
    const char *program, *folder, *arg1, *arg2;
} RUN[NUM_GAMES] = {
    { "nblood-bin", "Blood", NULL, NULL },
    { "nblood-bin", "Blood", "-ini", "CRYPTIC.INI" },
    { "rednukem", "Redneck/RR", NULL, NULL },
    { "rednukem", "Redneck/RRRA", NULL, NULL },
    { "pcexhumed", "PowerSlave", NULL, NULL },
};

// The GRP file of each Redneck Rampage game in Redneck/, by its size (that
// of the version rednukem knows, rr/src/grpscan.cpp), whatever its name
static char RedneckGrp[NUM_GAMES][NAME_MAX + 1];
static char RedneckPath[NUM_GAMES][NAME_MAX + 16];

static void check_redneck(mh_game *games)
{
    static const struct { int game; off_t size; } GRPS[] = {
        { RR, 141174222 },
        { RRRA, 191798609 },
    };
    static const char *files[NUM_GAMES][2];
    char path[PATH_MAX];
    struct dirent *e;
    struct stat st;
    size_t len;
    DIR *d;
    int i;

    games[RR].files = games[RRRA].files = MH_NOT_THERE;
    d = opendir("Redneck");
    if (!d)
        return;
    while ((e = readdir(d)) != NULL) {
        len = strlen(e->d_name);
        if (len < 4 || strcasecmp(e->d_name + len - 4, ".grp") != 0)
            continue;
        snprintf(path, sizeof(path), "Redneck/%s", e->d_name);
        if (stat(path, &st) != 0)
            continue;
        for (i = 0; i < 2; i++) {
            int g = GRPS[i].game;
            if (st.st_size != GRPS[i].size || RedneckGrp[g][0])
                continue;
            snprintf(RedneckGrp[g], sizeof(RedneckGrp[g]), "%s", e->d_name);
            snprintf(RedneckPath[g], sizeof(RedneckPath[g]), "Redneck/%s", e->d_name);
            files[g][0] = RedneckPath[g];
            files[g][1] = NULL;
            games[g].files = files[g];
        }
    }
    closedir(d);
}

int main(int argc, char **argv)
{
    mh_game games[NUM_GAMES];
    char program[PATH_MAX + 16], appdir[PATH_MAX], here[PATH_MAX];
    static char path_arg[PATH_MAX + 4];
    const char *args[10];
    char *slash;
    int pick, listed = 0, n = 0;

    (void)argc;
    if (!MH_Open()) {
        fprintf(stderr, "the core is not loaded\n");
        return EXIT_TO_MENU;
    }
    memcpy(games, GAMES, sizeof(games));
    check_redneck(games);
    // the game's logo and the lines under it ("Loading <name>..." the last)
    // come with the pick
    pick = MH_PickGame(games, NUM_GAMES, NULL, NO_GAME, &listed);
    if (pick < 0) {
        MH_Close();
        return EXIT_TO_MENU;
    }
    // quitting a game picked from the list shows the list again (exit code 42)
    if (listed)
        setenv("MISTER_HYBRID_FROM_LIST", "1", 1);
    // the logo stays and goes over to the game's program
    MH_Close();

    // the folder of this program, where the others and their .pk3 files are
    if (!realpath(argv[0], appdir)) {
        perror(argv[0]);
        return EXIT_TO_MENU;
    }
    slash = strrchr(appdir, '/');
    *slash = 0;
    snprintf(program, sizeof(program), "%s/%s", appdir, RUN[pick].program);

    if (pick == RR || pick == RRRA) {
        mkdir(RUN[pick].folder, 0755);
        setenv("DUKE3DGRP", RedneckGrp[pick], 1);
    }
    if (chdir(RUN[pick].folder) != 0 || !getcwd(here, sizeof(here))) {
        perror(RUN[pick].folder);
        return EXIT_TO_MENU;
    }
    args[n++] = program;
    args[n++] = "-usecwd";
    // Blood's command line takes a word that starts with "/" for a switch,
    // but not after "=" (blood/src/getopt.cpp)
    if (pick == BLOOD || pick == CRYPTIC) {
        snprintf(path_arg, sizeof(path_arg), "-j=%s", appdir);
        args[n++] = path_arg;
    } else {
        args[n++] = "-j";
        args[n++] = appdir;
    }
    if (pick == RR || pick == RRRA) {
        // the GRP files, in the folder above
        *strrchr(here, '/') = 0;
        args[n++] = "-j";
        args[n++] = here;
    }
    if (RUN[pick].arg1) {
        args[n++] = RUN[pick].arg1;
        args[n++] = RUN[pick].arg2;
    }
    args[n] = NULL;
    execv(program, (char *const *)args);
    perror(program);
    return EXIT_TO_MENU;
}
