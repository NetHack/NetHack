# Inactive level timestamp regression

Run in wizard mode on a disposable game. This checks save packing and
restore unpacking together; no changes to the save format are required.

1. Create a tame stone golem with `#wizgenesis` (`tame stone golem`).
   Use a stethoscope to confirm tameness 5. A stone golem does not eat,
   so starvation cannot account for the result.
2. Move far enough away that it will not follow, then change levels.
3. Spend 1,000 turns on the other level. Wizard-mode Lua can advance
   actual game turns with `nh.pushkey("."); nh.doturn();`. Disable
   random monster generation and hero hunger on that other level if
   needed to keep the experiment controlled.
4. Save and exit, restart, then return to the golem's level. Probe it
   with a stethoscope.

Expected: the golem is no longer tame. It may be peaceful or hostile;
that distinction is random and is not the assertion.

Repeat without saving, and with three save/restart cycles before returning.
All three cases must lose tameness. The unpatched code loses tameness only
in the no-save control and retains tameness 5 after saving.

For a controlled comparison, restore identical copies of a starting save
made before leaving the golem. Use a fresh copy for each case. No new save
can recover elapsed time already erased by an older executable.

Review invariants:

- Ordinary writes of the active level, including level changes and bones
  creation, use the current turn.
- Copying inactive levels during save packing and restore unpacking keeps
  the timestamp read by `getlev()`. The condition matches its guard that
  skips monster catch-up.
- Level flag timestamps use that same saved time base; serializing and
  undoing the conversion must leave their in-memory values unchanged.

This change does not fix the separate pet hungrytime reconstruction issue.
Corpse creation timestamps and ordinary corpse timer deadlines are not
changed by this patch.
