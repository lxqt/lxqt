#!/bin/bash

if [[ -z $1 ]]; then
	echo "Usage: $0 [TAG]"
	exit 1
fi

BASEDIR="$(readlink -f $(dirname $0))"
PROJECT="lxqt"
VERSION="$1"

mkdir -p "$BASEDIR/dist/$VERSION"

files=(
	"liblxqt"
	"lxqt-about"
	"lxqt-admin"
	"lxqt-config"
	"lxqt-globalkeys"
	"lxqt-notificationd"
	"lxqt-openssh-askpass"
	"lxqt-panel"
	"lxqt-policykit"
	"lxqt-powermanagement"
	"lxqt-qtplugin"
	"lxqt-runner"
	"lxqt-session"
	"lxqt-sudo"
	"pcmanfm-qt"
)

for f in "${files[@]}"; do
	echo "Packaging $f"
	cd "$BASEDIR/$f"
	git tag -fsm "Release v$VERSION" $1 # && git push --tags --force

	git archive --prefix=$f-$VERSION/ $1 -o "../dist/$1/$f-$1.tar.gz"
	gpg --armor --detach-sign "../dist/$VERSION/$f-$1.tar.gz"
	git archive --prefix=$f-$VERSION/ $1 -o "../dist/$1/$f-$1.tar.xz"
	gpg --armor --detach-sign "../dist/$VERSION/$f-$1.tar.xz"
done

echo "Done. Uploading..."

cd "$BASEDIR/dist/$VERSION"
sha1sum --tag *-$VERSION.*z > CHECKSUMS
sha256sum --tag *-$VERSION.*z >> CHECKSUMS
scp -r "$BASEDIR/dist/$VERSION" "downloads.lxqt.org:/srv/downloads.lxqt.org/$PROJECT/"
