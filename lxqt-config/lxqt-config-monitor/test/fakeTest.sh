export KSCREEN_BACKEND=Fake
export KSCREEN_BACKEND_ARGS=TEST_DATA=`pwd`/multipleoutput3.json
killall kscreen_backend_launcher
#`find /usr -name 'kscreen_backend_launcher'` 
lxqt-config-monitor

