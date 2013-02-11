#
# mkapp example application: music player
#

# Define user interface modules
define glade_gui  mkglade   player.glade;
define html_gui   mkhtml    file://./player.html
                            -w 400 -h 75 -t -b;

define controller mkmachine controller.mac;
define data       mkstore   -t '\$(\w+)' -a /tmp/player.app.data;
define mpg321     mpg321    -R " " --skip-printing-frames=100;

# Bind glade_gui to the controller and obey to the controller's commands
bind glade_gui  controller;
bind html_gui   controller;
bind mpg321     controller;

bind controller data;
obey data;

# Attach an ID3-info updater to the html gui
define id3 ./id3.sh;
bind   id3 html_gui;
run    id3;

# Run the modules
run data;
run glade_gui;
run controller;

# Debug
#listen glade_gui;
#listen html_gui;
#listen controller;
#listen data;
