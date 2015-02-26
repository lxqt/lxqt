#!/bin/bash

if [[ -z $1 ]]; then
	echo "Usage: $0 [TAG]"
	exit 1
fi

mkdir -p "dist/$1"

files=(
	"liblxqt"
	"liblxqt-mount"
	"lxqt-about"
	"lxqt-admin"
	"lxqt-common"
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
	"pcmanfm-qt"
)

for f in ${files[@]}; do
	cd "$f"
	git tag -fsm "Release v$1" $1 && git push --tags --force

	echo "Packaging $f"
	git archive --prefix=$f-$1/ $1 -o "../dist/$1/$f-$1.tar.gz"
	gpg --armor --detach-sign "../dist/$1/$f-$1.tar.gz"
	git archive --prefix=$f-$1/ $1 -o "../dist/$1/$f-$1.tar.xz"
	gpg --armor --detach-sign "../dist/$1/$f-$1.tar.xz"
	cd ..
done

echo "Done. Uploading..."

cd "dist/$1"
sha1sum --tag *-$1.*z > CHECKSUMS
sha256sum --tag *-$1.*z >> CHECKSUMS
cd ..
scp -r $1 lxde.org:/var/www/lxqt/downloads/lxqt
