#! /usr/bin/env bash
(
cd $(dirname $(which "$0"))/.. || exit 1
####################################################################


echo ========== cleanup ==========
rm -vf uriparser-*.tar.* uriparser-*.zip 2> /dev/null
rm -vRf uriparser-* 2> /dev/null

echo
echo ========== bootstrap ==========
./autogen.sh --download || exit 1

if [[ ! -f Makefile ]]; then
echo
echo ========== configure ==========
./configure --enable-doc || exit 1
fi

echo
echo ========== make distcheck ==========
make -j10 distcheck || exit 1

echo
echo ========== package docs ==========
./doc/release.sh || exit 1

####################################################################
)
res=$?
[ $res = 0 ] || exit $res

cat <<'CHECKLIST'

Fine.


Have you
* run ./edit_version.sh
* updated the soname
* updated file lists
  - Makefile.am
  - Code::Blocks
  - Visual Studio 2005
* searched for TODO inside code using
  grep -R 'TODO' include/* lib/* test/*
?

If so ..
* upload release with ReleaseForge
* announce through ..
  - Blog
  - Mailing lists
  - Freshmeat
  - SourceForge news
* upload doc
* update doc to website

CHECKLIST
exit $res
