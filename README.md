# People_Track:
 A DeepStream-based tracking and detection platform for embedded architectures.
 
 # App description:
This application allows us to perform detection of people and their tracking. We can know in every instant of the scene the coordinates of each person and the distance that it has from other people, we can reconstruct (through the coordinates) the path that this person has made. Briefly, thanks to this application it is literally possible to have a mapping of the positions that each person has taken within the scene.

# To run the app:
To run People_Track you must have correctly installed deepstream 5.0 with right /opt root directory.
All config files, header files, networks are provided in this folders.
Move to /sources/apps/my_apps/People_Track and follow the README instruction.

# Custom plugin:
An example of g_stream custom plugin is also provided in this repo. Its purpose remains mainly to show how a plugin with a customized logic can be integrated within a deepstream pipeline. As it is, the plugin allows us to print on each person, the distance between them and the closest person. We can also use the gst-dsexample default features like "Blurring" of the detected classes.
To integrate the custom plugin in People_Track, move to: /sources/gst-plugins/gst-dsexample and follow the README instructions

http://github.com - automatic!
[GitHub](http://github.com)
