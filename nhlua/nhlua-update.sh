#!/bin/sh

# Update nhlua from a new Lua tar.gz file.
#
# How to run:
#  cd nhlua
#  git checkout DESTINATION-BRANCH
#  ./nhlua-update.sh PATH_TO_lua-...tar.gz

nhcurrent=`git branch --show-current`

#NOTES:
#Q: Do we want the test suite?  It's in git directory testes [sic] plus
#   at least one file in src.
#
#Q: Do we want to be able to import changes (bug fixes) from git?
#
#Q: How do we handle files not included in a new tarball?
#A: We don't.  We'd need to compare 'tar t' to 'ls -R'. XXX

#set +x

# Make sure we're in the right place
cd ../nhlua || { echo "Must be run from nhlua" ; exit 1; };

# This file lives on the current branch, but we're going to check out
# branch lua on top of it, so we need to copy this file somewhere and
# then exec it with our original arguments, but marked so we don't loop.
#
# For testing:
#   cp nhlua-update.sh nhlua-update.sh.tmp
#   edit nhlua-update.sh.tmp
#   ./nhlua-update.sh.tmp -- DOWNLOAD-TARBALL.tar.gz
# when done, copy it back and commit.
arg=shift
if [ x$1 != 'x--' ]; then
    cp $0 $0.tmp
    chmod +x $0.tmp
    exec $0.tmp -- "$@"
fi

version=`echo $2|sed -E -n -e 's/(.*)lua-(.*).tar.gz/\2/p'`
echo "=== Working on $version."

# Make sure the worktree is clean here so we add only the right files
# from the tarball, if any.
if [ `git status --porcelain=2 | grep '^?' | wc -l` != 0 ]; then
    echo "Must be run in a clean worktree.  Giving up.";
    exit 1
fi

git checkout PUC-RIO-Lua || exit 1

echo "=== Unpack $2."
(cd lua; tar -x --strip-components 1 -f $2) || exit 1

# Add any new files.
echo "=== Check for new files."
git -c advice.addEmptyPathspec=false add -N `git status --porcelain=2 . |sed -E -n -e 's/^\? (.*)/\1/p'` || exit 1

echo "=== Commit changes?"
if [ `git status -s`x == x ]; then
    echo "Nothing to commit."
else
    git commit -a -m "Update clean tree to Lua version $version" || exit 1
fi

# now update the destination branch
echo "=== Checkout $nhcurrent"
git checkout $nhcurrent || exit 1

echo "=== Merge into original branch."
echo '  If the merge succeeds, do "git push".'
echo '  If the merge fails, clean up the mess then "git merge --continue".'
git merge --no-ff --no-commit PUC-RIO-Lua || exit 1

echo "=== Commit changes."
git commit -a -v -m "Update to Lua version $version" || exit 1

echo ""
echo 'If everything is OK, do a "git push".  Otherwise clean up the mess.'

exit 0
