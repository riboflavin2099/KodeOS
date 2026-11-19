/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file test_doom.c
 * @brief Host checks for WAD identification, button layout, and a directory scan.
 *
 * No SDK and no commercial WAD. A path on the command line, or DOOM_WAD_DIR,
 * is scanned after the synthetic checks. Known file names in that directory
 * must identify as the game this table expects. Other files are printed.
 */
#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "doom_layout.h"
#include "doom_wad.h"

static int s_failed = 0;

static void expect(int cond, const char *what)
{
    if ( !cond )
    {
        fprintf(stderr, "FAIL %s\n", what);
        s_failed++;
    }
}

static void header(uint8_t *hdr, const char *magic, uint32_t lumps, uint32_t off)
{
    memset(hdr, 0, 12);
    memcpy(hdr, magic, 4);
    hdr[4] = (uint8_t)(lumps & 0xffU);
    hdr[5] = (uint8_t)((lumps >> 8) & 0xffU);
    hdr[8] = (uint8_t)(off & 0xffU);
    hdr[9] = (uint8_t)((off >> 8) & 0xffU);
    hdr[10] = (uint8_t)((off >> 16) & 0xffU);
    hdr[11] = (uint8_t)((off >> 24) & 0xffU);
}

static void lump_name(char out[8], const char *name)
{
    size_t i = 0U;

    memset(out, 0, 8);
    for ( i = 0U; (8U > i) && ('\0' != name[i]); i++ )
    {
        out[i] = name[i];
    }
}

/**
 * @brief Write a tiny WAD whose directory lists the given lump names.
 */
static int write_wad(const char *path, const char *magic, const char **lumps, int n)
{
    FILE *fp = fopen(path, "wb");
    uint8_t hdr[12];
    int i = 0;
    uint32_t dir_off = 12U;

    if ( NULL == fp )
    {
        return -1;
    }
    header(hdr, magic, (uint32_t)n, dir_off);
    if ( 1 != fwrite(hdr, 12, 1, fp) )
    {
        fclose(fp);
        return -1;
    }
    for ( i = 0; n > i; i++ )
    {
        uint8_t ent[16];

        memset(ent, 0, sizeof(ent));
        lump_name((char *)&ent[8], lumps[i]);
        if ( 1 != fwrite(ent, 16, 1, fp) )
        {
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

/**
 * @brief Identify one real file. Returns 0 on a readable WAD header.
 */
static int identify_path(const char *path, const char *name, doom_id_t *id, uint32_t *lumps_out)
{
    FILE *fp = fopen(path, "rb");
    uint8_t hdr[12];
    uint32_t nlumps = 0U;
    uint32_t off = 0U;
    uint32_t bits = 0U;
    uint32_t seen = 0U;
    struct stat st;
    doom_wad_kind_t kind = DOOM_WAD_BAD;

    if ( NULL == fp )
    {
        return -1;
    }
    if ( 0 != fstat(fileno(fp), &st) )
    {
        fclose(fp);
        return -1;
    }
    if ( 1 != fread(hdr, 12, 1, fp) )
    {
        fclose(fp);
        return -1;
    }
    kind = doom_wad_kind(hdr, 12U, &nlumps, &off);
    if ( (DOOM_WAD_BAD != kind) && (0U < nlumps) && (0U < off) )
    {
        uint32_t cap = nlumps;

        if ( 8192U < cap )
        {
            cap = 8192U;
        }
        if ( 0 != fseek(fp, (long)off, SEEK_SET) )
        {
            fclose(fp);
            return -1;
        }
        while ( seen < cap )
        {
            uint8_t ent[16];

            if ( 1 != fread(ent, 16, 1, fp) )
            {
                break;
            }
            bits = doom_wad_note_entry(bits, ent);
            seen++;
        }
    }
    fclose(fp);
    *id = doom_wad_identify(name, kind, bits, (uint32_t)st.st_size);
    if ( NULL != lumps_out )
    {
        *lumps_out = bits;
    }
    return 0;
}

static void check_identity(void)
{
    uint8_t hdr[12];
    uint32_t lumps = 0U;
    uint32_t off = 0U;
    char raw[8];
    uint32_t bits = 0U;
    doom_id_t id;

    expect(DOOM_WAD_BAD == doom_wad_kind(NULL, 12U, &lumps, &off), "NULL header");
    header(hdr, "IWAD", 4U, 12U);
    expect(DOOM_WAD_IWAD == doom_wad_kind(hdr, sizeof(hdr), &lumps, &off), "IWAD");
    expect((4U == lumps) && (12U == off), "directory fields");
    header(hdr, "PWAD", 1U, 32U);
    expect(DOOM_WAD_PWAD == doom_wad_kind(hdr, sizeof(hdr), &lumps, &off), "PWAD");

    lump_name(raw, "map01");
    bits = doom_wad_note_lump(0U, raw);
    expect(0U != (bits & DOOM_LUMP_MAP01), "MAP01 any case");
    expect(bits == doom_wad_note_lump(bits, NULL), "NULL lump keeps bits");

    id = doom_wad_identify("SD/DOOM/DOOM1.WAD", DOOM_WAD_IWAD, 0U, 0U);
    expect(id.show && (DOOM_ROLE_IWAD == id.role), "doom1 shows");
    expect(0 == strcmp(id.title, "DOOM SHARE"), "doom1 title");
    expect(DOOM_BASE_DOOM_SW == id.base, "doom1 is not a SIGIL base");

    id = doom_wad_identify("doom.wad", DOOM_WAD_IWAD, DOOM_LUMP_E4M1 | DOOM_LUMP_M_DOOM, 0U);
    expect(0 == strcmp(id.title, "ULTIMATE DOOM"), "ultimate from E4M1");
    expect(DOOM_BASE_DOOMU == id.base, "ultimate base");
    expect(0 == strcmp(doom_wad_art_lump(&id, DOOM_LUMP_E4M1 | DOOM_LUMP_M_DOOM), "M_DOOM"),
           "M_DOOM is the Doom art lump");

    id = doom_wad_identify("doom.wad", DOOM_WAD_IWAD, DOOM_LUMP_E3M1, 0U);
    expect((0 == strcmp(id.title, "DOOM")) && (DOOM_BASE_DOOM == id.base), "registered doom.wad");

    id = doom_wad_identify("doom2.wad", DOOM_WAD_IWAD, DOOM_LUMP_TITLEPIC, 0U);
    expect(id.show && (DOOM_BASE_DOOM2 == id.base), "doom2");
    expect(0 == strcmp(doom_wad_art_lump(&id, DOOM_LUMP_TITLEPIC), "TITLEPIC"), "TITLEPIC");

    id = doom_wad_identify("heretic.wad", DOOM_WAD_IWAD, DOOM_LUMP_EXTENDED | DOOM_LUMP_M_HTIC, 0U);
    expect(0 == strcmp(id.title, "HERETIC EXT"), "heretic extended");
    expect(0 == strcmp(doom_wad_art_lump(&id, DOOM_LUMP_M_HTIC), "M_HTIC"), "M_HTIC");

    id = doom_wad_identify("hexen.wad", DOOM_WAD_IWAD, 0U, 0U);
    expect(id.show && (DOOM_ENGINE_HEXEN == id.engine), "hexen by name");

    id = doom_wad_identify("tnt.wad", DOOM_WAD_IWAD, 0U, 0U);
    expect((DOOM_BASE_TNT == id.base) && id.show, "tnt");
    id = doom_wad_identify("plutonia.wad", DOOM_WAD_IWAD, 0U, 0U);
    expect(DOOM_BASE_PLUTONIA == id.base, "plutonia by name");

    id = doom_wad_identify("freedoom2.wad", DOOM_WAD_IWAD, 0U, 0U);
    expect(id.show && (DOOM_BASE_DOOM2 == id.base), "freedoom2 is a Doom II base");

    id = doom_wad_identify("strife1.wad", DOOM_WAD_IWAD, DOOM_LUMP_MAP01, 0U);
    expect(!id.show && (0 == strcmp(id.title, "STRIFE")), "strife is named and hidden");

    id = doom_wad_identify("renamed.wad", DOOM_WAD_IWAD,
                           DOOM_LUMP_MAP01 | DOOM_LUMP_BEHAVIOR, 0U);
    expect(DOOM_ENGINE_HEXEN == id.engine, "hexen by lumps");

    id = doom_wad_identify("renamed.wad", DOOM_WAD_IWAD,
                           DOOM_LUMP_MAP01 | DOOM_LUMP_CAMO1 | DOOM_LUMP_D_RUNNIN, 0U);
    expect(0 == strcmp(id.title, "PLUTONIA"), "plutonia lumps beat D_RUNNIN");

    id = doom_wad_identify("renamed.wad", DOOM_WAD_IWAD, 0U, 14604584U);
    expect(0 == strcmp(id.title, "DOOM II"), "doom2 by published size");

    id = doom_wad_identify("renamed.wad", DOOM_WAD_PWAD, 0U, 34793603U);
    expect(id.show && (DOOM_ROLE_ADDON == id.role), "id1 size is an add-on button");
    expect(0 == strcmp(id.title, "LEGACY OF RUST"), "id1 size title");

    id = doom_wad_identify("sigil.wad", DOOM_WAD_PWAD, DOOM_LUMP_E5M1, 0U);
    expect(id.show && (DOOM_ROLE_ADDON == id.role) && (DOOM_BASE_DOOMU == id.base),
           "SIGIL needs Ultimate Doom");
    expect(0 == strcmp(id.title, "SIGIL"), "SIGIL title");

    id = doom_wad_identify("SIGIL2.WAD", DOOM_WAD_PWAD, DOOM_LUMP_E6M1, 0U);
    expect(0 == strcmp(id.title, "SIGIL II"), "SIGIL II any case");

    id = doom_wad_identify("episode5.wad", DOOM_WAD_PWAD, DOOM_LUMP_E5M1, 0U);
    expect(0 == strcmp(id.title, "SIGIL"), "E5M1 on a PWAD is SIGIL");

    id = doom_wad_identify("id1.wad", DOOM_WAD_PWAD, DOOM_LUMP_MAP01, 0U);
    expect(id.show && (0 == strcmp(id.title, "LEGACY OF RUST")), "id1.wad by name");
    expect(DOOM_BASE_DOOM2 == id.base, "Legacy of Rust needs Doom II");

    id = doom_wad_identify("id1-res.wad", DOOM_WAD_PWAD, DOOM_LUMP_MAP01, 0U);
    expect(!id.show && (0 == strcmp(id.title, "ID1 RESOURCES")), "id1-res is not id1");

    id = doom_wad_identify("extras.wad", DOOM_WAD_PWAD, 0U, 0U);
    expect(!id.show, "extras.wad is hidden");

    id = doom_wad_identify("nerve.wad", DOOM_WAD_PWAD, DOOM_LUMP_MAP01, 0U);
    expect(id.show && (0 == strcmp(id.title, "NO REST")), "nerve.wad");

    id = doom_wad_identify("masterlevels.wad", DOOM_WAD_PWAD, 0U, 0U);
    expect(id.show && (DOOM_ROLE_ADDON == id.role), "master levels");

    id = doom_wad_identify("doom2.wad", DOOM_WAD_PWAD, DOOM_LUMP_MAP01, 0U);
    expect(!id.show && (0 == strcmp(id.title, "ADD-ON")), "a PWAD named doom2 is not the IWAD");

    id = doom_wad_identify("notes.txt", DOOM_WAD_BAD, 0U, 0U);
    expect(!id.show && (0 == strcmp(id.title, "UNKNOWN")), "bad magic");
}

static void check_layout(void)
{
    doom_layout_t lay;
    int hit = 0;

    doom_layout_build(0U, 0U, 410, 502, &lay);
    expect(0U == lay.visible, "no games, no buttons");

    doom_layout_build(1U, 0U, 410, 502, &lay);
    expect((1U == lay.visible) && (1U == lay.cols), "one game is one button");
    expect((lay.rect[0].w > 200) && (lay.rect[0].h > 150), "the single button is large");
    hit = doom_layout_hit(&lay, (int16_t)(lay.rect[0].x + 4), (int16_t)(lay.rect[0].y + 4));
    expect(0 == hit, "hit the only button");
    expect(-1 == doom_layout_hit(&lay, 0, 0), "the corner is not the button");

    doom_layout_build(2U, 0U, 410, 502, &lay);
    expect((2U == lay.visible) && (1U == lay.cols), "two games stack");

    doom_layout_build(5U, 0U, 410, 502, &lay);
    expect((4U == lay.visible) && (2U == lay.cols) && (0U == lay.origin), "grid shows four");
    doom_layout_build(5U, 2U, 410, 502, &lay);
    expect((2U == lay.origin) && (3U == lay.visible), "scroll reveals the rest");
    hit = doom_layout_hit(&lay, (int16_t)(lay.rect[0].x + 2), (int16_t)(lay.rect[0].y + 2));
    expect(2 == hit, "hit index includes the scroll origin");
}

static int scan_dir(const char *dir, int strict)
{
    DIR *d = opendir(dir);
    struct dirent *ent = NULL;
    int saw = 0;

    if ( NULL == d )
    {
        fprintf(stderr, "FAIL open %s: %s\n", dir, strerror(errno));
        return 1;
    }
    while ( NULL != (ent = readdir(d)) )
    {
        size_t n = 0U;
        char path[512];
        doom_id_t id;
        uint32_t bits = 0U;
        const char *expect_title = NULL;

        if ( (0 == strcmp(ent->d_name, ".")) || (0 == strcmp(ent->d_name, "..")) )
        {
            continue;
        }
        n = strlen(ent->d_name);
        if ( (4U > n) || (0 != strcasecmp(ent->d_name + (n - 4U), ".wad")) )
        {
            continue;
        }
        if ( sizeof(path) <= (strlen(dir) + n + 2U) )
        {
            fprintf(stderr, "FAIL path too long: %s\n", ent->d_name);
            s_failed++;
            continue;
        }
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        if ( 0 != identify_path(path, ent->d_name, &id, &bits) )
        {
            fprintf(stderr, "FAIL read %s\n", path);
            s_failed++;
            continue;
        }
        saw++;
        printf("wad %s -> %s role %u base %u show %u art %s\n",
               ent->d_name, id.title, (unsigned)id.role, (unsigned)id.base,
               id.show ? 1U : 0U,
               (NULL != doom_wad_art_lump(&id, bits)) ? doom_wad_art_lump(&id, bits) : "-");
        if ( strict )
        {
            if ( 0 == strcasecmp(ent->d_name, "doom1.wad") )
            {
                expect_title = "DOOM SHARE";
            }
            else if ( 0 == strcasecmp(ent->d_name, "sigil.wad") )
            {
                expect_title = "SIGIL";
            }
            else if ( 0 == strcasecmp(ent->d_name, "id1-res.wad") )
            {
                expect_title = "ID1 RESOURCES";
            }
            if ( (NULL != expect_title) && (0 != strcmp(id.title, expect_title)) )
            {
                fprintf(stderr, "FAIL %s identified as %s\n", ent->d_name, id.title);
                s_failed++;
            }
        }
        else
        {
            /* A tester's directory. Known campaign names must not come back unknown. */
            static const struct { const char *file; const char *title; } known[] = {
                { "doom.wad", "ULTIMATE DOOM" },
                { "doom1.wad", "DOOM SHARE" },
                { "doom2.wad", "DOOM II" },
                { "tnt.wad", "TNT" },
                { "plutonia.wad", "PLUTONIA" },
                { "heretic.wad", "HERETIC" },
                { "hexen.wad", "HEXEN" },
                { "sigil.wad", "SIGIL" },
                { "sigil2.wad", "SIGIL II" },
                { "id1.wad", "LEGACY OF RUST" },
                { "nerve.wad", "NO REST" },
                { "freedoom1.wad", "FREEDOOM 1" },
                { "freedoom2.wad", "FREEDOOM 2" },
            };
            size_t k = 0U;

            for ( k = 0U; (sizeof(known) / sizeof(known[0])) > k; k++ )
            {
                if ( 0 == strcasecmp(ent->d_name, known[k].file)
                    && (0 != strcmp(id.title, known[k].title)) )
                {
                    /* doom.wad without E4M1 is registered Doom, which is still Doom. */
                    if ( (0 == strcasecmp(ent->d_name, "doom.wad"))
                        && (0 == strcmp(id.title, "DOOM")) )
                    {
                        continue;
                    }
                    if ( (0 == strcasecmp(ent->d_name, "heretic.wad"))
                        && ((0 == strcmp(id.title, "HERETIC EXT"))
                            || (0 == strcmp(id.title, "HERETIC SW"))) )
                    {
                        continue;
                    }
                    fprintf(stderr, "FAIL %s identified as %s, expected %s\n",
                            ent->d_name, id.title, known[k].title);
                    s_failed++;
                }
            }
        }
    }
    closedir(d);
    if ( strict && (3 != saw) )
    {
        fprintf(stderr, "FAIL synthetic directory saw %d wads\n", saw);
        s_failed++;
    }
    return 0;
}

static void check_directory(void)
{
    char dir[] = "/tmp/doom-wad-test-XXXXXX";
    char path[256];
    const char *e4[] = { "E4M1", "M_DOOM" };
    const char *e5[] = { "E5M1" };
    const char *res[] = { "MAP01" };

    if ( NULL == mkdtemp(dir) )
    {
        fprintf(stderr, "FAIL mkdtemp\n");
        s_failed++;
        return;
    }
    snprintf(path, sizeof(path), "%s/doom.wad", dir);
    expect(0 == write_wad(path, "IWAD", e4, 2), "write doom.wad");
    snprintf(path, sizeof(path), "%s/sigil.wad", dir);
    expect(0 == write_wad(path, "PWAD", e5, 1), "write sigil.wad");
    snprintf(path, sizeof(path), "%s/id1-res.wad", dir);
    expect(0 == write_wad(path, "PWAD", res, 1), "write id1-res.wad");
    scan_dir(dir, 1);
    snprintf(path, sizeof(path), "%s/doom.wad", dir);
    remove(path);
    snprintf(path, sizeof(path), "%s/sigil.wad", dir);
    remove(path);
    snprintf(path, sizeof(path), "%s/id1-res.wad", dir);
    remove(path);
    rmdir(dir);
}

int main(int argc, char **argv)
{
    const char *extra = NULL;

    check_identity();
    check_layout();
    check_directory();
    if ( 1 < argc )
    {
        extra = argv[1];
    }
    else
    {
        extra = getenv("DOOM_WAD_DIR");
    }
    if ( (NULL != extra) && ('\0' != extra[0]) )
    {
        printf("scanning %s\n", extra);
        scan_dir(extra, 0);
    }
    if ( 0 == s_failed )
    {
        printf("OK doom\n");
    }
    else
    {
        fprintf(stderr, "%d failed\n", s_failed);
    }
    return (0 == s_failed) ? 0 : 1;
}
