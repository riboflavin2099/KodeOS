#!/usr/bin/env python3
"""Prefix globals that a game tree defines, then restore libc and shared names.

objcopy --prefix-symbols renames undefined references too. After the prefix,
any heretic_foo that the tree does not define is renamed back to foo. Calls
inside the tree stay prefixed, so Doom, Heretic and Hexen can live in one link.
"""

import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile


def symbols(nm, obj, mode):
    out = subprocess.check_output([nm, "-g", mode, obj], text=True, errors="replace")
    names = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 1 and parts[-1] not in {"", "U"} and not parts[-1].endswith(":"):
            if len(parts) == 1:
                continue
            names.add(parts[-1])
    return names


def already(nm, archive, prefix):
    try:
        out = subprocess.check_output(
            [nm, "-g", "--defined-only", archive], text=True, errors="replace"
        )
    except subprocess.CalledProcessError:
        return False
    token = prefix
    return any(line.split()[-1].startswith(token) for line in out.splitlines() if line.split())


def members(ar, archive, tmp):
    subprocess.check_call([ar, "x", os.path.abspath(archive)], cwd=tmp)
    objs = sorted(glob.glob(os.path.join(tmp, "*")))
    return [obj for obj in objs if not obj.endswith(".txt")]


def repack(ar, archive, objs):
    os.remove(archive)
    subprocess.check_call([ar, "rcs", archive, *objs])


def apply_prefix(args):
    if already(args.nm, args.archive, args.prefix):
        print(f"prefix {args.prefix} already applied to {args.archive}")
        return 0
    tmp = tempfile.mkdtemp(prefix="doom-prefix-")
    try:
        objs = members(args.ar, args.archive, tmp)
        if not objs:
            print("prefix: archive has no members", file=sys.stderr)
            return 1
        defined = set()
        for obj in objs:
            defined |= symbols(args.nm, obj, "--defined-only")
        for obj in objs:
            subprocess.check_call([args.objcopy, f"--prefix-symbols={args.prefix}", obj])
        back = {}
        for obj in objs:
            for name in symbols(args.nm, obj, "--undefined-only"):
                if not name.startswith(args.prefix):
                    continue
                orig = name[len(args.prefix) :]
                if orig and orig not in defined:
                    back[name] = orig
        if back:
            listing = os.path.join(tmp, "redef.txt")
            with open(listing, "w", encoding="ascii") as handle:
                for old, new in sorted(back.items()):
                    handle.write(f"{old} {new}\n")
            for obj in objs:
                subprocess.check_call([args.objcopy, f"--redefine-syms={listing}", obj])
        repack(args.ar, args.archive, objs)
        print(
            f"prefixed {len(defined)} {args.prefix} symbols, "
            f"restored {len(back)} external names"
        )
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return 0


def section_names(objdump, obj):
    out = subprocess.check_output([objdump, "-h", obj], text=True, errors="replace")
    names = []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[0].isdigit() and parts[1].startswith("."):
            names.append(parts[1])
    return names


def move_bss(args):
    tmp = tempfile.mkdtemp(prefix="doom-bss-")
    try:
        objs = members(args.ar, args.archive, tmp)
        moved = 0
        for obj in objs:
            renames = []
            for name in section_names(args.objdump, obj):
                if name.startswith(".bss"):
                    renames.append((name, ".ext_ram.bss" + name[len(".bss") :]))
                elif name.startswith(".sbss"):
                    renames.append((name, ".ext_ram.bss" + name[len(".sbss") :]))
            if not renames:
                continue
            cmd = [args.objcopy]
            for old, new in renames:
                cmd.append(f"--rename-section={old}={new}")
            cmd.append(obj)
            subprocess.check_call(cmd)
            moved += len(renames)
        if moved:
            repack(args.ar, args.archive, objs)
        print(f"moved {moved} bss sections in {os.path.basename(args.archive)} to PSRAM")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", required=True)
    parser.add_argument("--prefix", default="")
    parser.add_argument("--move-bss", action="store_true")
    parser.add_argument("--nm", default="nm")
    parser.add_argument("--objcopy", default="objcopy")
    parser.add_argument("--objdump", default="objdump")
    parser.add_argument("--ar", default="ar")
    args = parser.parse_args()
    if not args.prefix and not args.move_bss:
        print("prefix_objs: pass --prefix and/or --move-bss", file=sys.stderr)
        return 1
    if args.prefix:
        status = apply_prefix(args)
        if status != 0:
            return status
    if args.move_bss:
        return move_bss(args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
