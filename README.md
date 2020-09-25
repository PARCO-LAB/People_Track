# :computer: People_Track:
***A DeepStream-based tracking and detection platform for embedded architectures.*** 


Below we can find an example of the output generated:

![](https://s1.gifyu.com/images/ezgif.com-optimize-21a2f196fe29bbfb3.gif)

In this repo, a [resnet10](https://github.com/PARCO-LAB/People_Track/tree/master/networks/models/Primary_Detector) network is provided for primary detector inference. 
It is however possible to change the network used by modifying the [pgie_config.txt](https://github.com/PARCO-LAB/People_Track/blob/master/sources/apps/my_apps/People%20Track/People_Track_pgie_config.txt) file. 

Similarly for tracking, by modifying the [People_Track_tracker_config.txt](https://github.com/PARCO-LAB/People_Track/blob/master/sources/apps/my_apps/People%20Track/People_Track_tracker_config.txt) file it is possible to choose between 2 possibilities. 
For more: [NvTracker](https://docs.nvidia.com/metropolis/deepstream/dev-guide/index.html#page/DeepStream%20Plugins%20Development%20Guide/deepstream_plugin_details.3.02.html#wwpID0E0N20HA)

 # :computer: App description:
This application allows us to perform detection of people and their tracking. We can know in every instant of the scene the coordinates of each person and the distance that it has from other people, we can reconstruct (through the coordinates) the path that this person has made. \nBriefly, thanks to this application it is literally possible to have a mapping of the positions that each person has taken within the scene.

# :computer: To run the app:
To run People_Track you must have correctly installed deepstream 5.0 with right /opt root directory.
All config files, header files, networks are provided in this folders.
Move to [People_Track](https://github.com/PARCO-LAB/People_Track/tree/master/sources/apps/my_apps/People%20Track) and follow the [README](https://github.com/PARCO-LAB/People_Track/blob/master/sources/apps/my_apps/People%20Track/README) instruction.

# :computer: Custom plugin:
An example of g_stream custom plugin is also provided in this repo. Its purpose remains mainly to show how a plugin with a customized logic can be integrated within a deepstream pipeline. As it is, the plugin allows us to print on each person, the distance between them and the closest person:

![](https://s1.gifyu.com/images/ezgif.com-optimize-59dc532e5f7902dff.gif)

We can also use the gst-dsexample default features like "Blurring" of the detected classes:

![](https://s1.gifyu.com/images/ezgif.com-optimize-346931188fde4fb17.gif)

To integrate the custom plugin in People_Track, move to: [gst-dsexample](https://github.com/PARCO-LAB/People_Track/tree/master/sources/gst-plugins/gst-dsexample) and follow the [README](https://github.com/PARCO-LAB/People_Track/blob/master/sources/gst-plugins/gst-dsexample/README) instructions


