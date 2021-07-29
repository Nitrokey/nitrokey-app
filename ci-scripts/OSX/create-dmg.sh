FNAME=$1
VER=$2
echo create-dmg:
bash ~/projects/create-dmg/create-dmg --volname "Nitrokey App" --window-pos 200 120 --window-size 800 400 --app-drop-link 600 0 --no-internet-enable "Nitrokey App ${VER}.dmg"  "${FNAME}"