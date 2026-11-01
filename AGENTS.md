# Notes for people and agents changing this repo

Every change must add an entry to `VERSION_CHANGES.txt` at the
repository root. Put the newest entry first. Use the date as
YYYY-MM-DD and one or two plain-English lines saying what changed.
`RIGHTEOUSHACK_CHANGES.md` is the detailed inventory of the
original customizations; do not use it as the changelog.

If a change touches food, monster meat, cooking, or which monsters
leave meat, regenerate `docs/RighteousHack_Food.xlsx` and commit
the new workbook:

    python3 tools/gen_food_xlsx.py

The script reads `include/meatnoun.h`, `include/objects.h`, and
`include/monsters.h`. Do not edit the workbook by hand.
