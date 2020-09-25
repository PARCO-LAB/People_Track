#include <gst/gst.h>

#include <glib.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include "gstnvdsmeta.h"

#define PGIE_CONFIG_FILE "People_Track_pgie_config.txt"
#define MAX_DISPLAY_LEN 64

#define TRACKER_CONFIG_FILE "People_Track_tracker_config.txt"
#define MAX_TRACKING_ID_LEN 16

#define PGIE_CLASS_ID_PERSON 2

/* The muxer output resolution must be set if the input streams will be of
 * different resolution. The muxer will scale all the input frames to this
 * resolution. */
#define MUXER_OUTPUT_WIDTH 1920
#define MUXER_OUTPUT_HEIGHT 1080

/* Muxer batch formation timeout, for e.g. 40 millisec. Should ideally be set
 * based on the fastest source's framerate. */
#define MUXER_BATCH_TIMEOUT_USEC 40000
#define NUM_TAVOLI 10

/* Global switch to set livecam option */
int LIVECAM = 1;


struct Table {
  int id;
  int x;
  int y;
};


/* Structure containing a list of all tables */
struct Sala {
  struct Table * Tavoli[NUM_TAVOLI];
};

gint frame_number = 0;
gchar pgie_classes_str[1][32] = {
  "Person"
};

/* gie_unique_id is one of the properties in the above People_Track_sgiex_config.txt
 * files. These should be unique and known when we want to parse the Metadata
 * respective to the sgie labels. Ideally these should be read from the config
 * files but for brevity we ensure they are same. */

guint People_Track_unique_id = 2;

//#########################################################################

/* This function allows us to instantiate the plugins needed to read the data stream coming from a telecameter connected to the board. */

static GstElement *
  create_camera_source_bin(guint index, gchar * uri) {
    GstElement * bin = NULL;
    GstCaps * caps = NULL;
    gboolean ret = FALSE;

    gchar bin_name[16] = {};
    g_snprintf(bin_name, 15, "source-bin-%02d", index);
    bin = gst_bin_new(bin_name);

    GstElement * src_elem = gst_element_factory_make("v4l2src", "src_elem");
    if (!src_elem) {
      g_printerr("Could not create 'src_elem'\n");
      return NULL;
    }

    GstElement * cap_filter = gst_element_factory_make("capsfilter", "src_cap_filter");
    if (!cap_filter) {
      g_printerr("Could not create 'src_cap_filter'\n");
      return NULL;
    }

    caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "NV12",
      "width", G_TYPE_INT, 1280, "height", G_TYPE_INT, 960,
      "framerate", GST_TYPE_FRACTION, 30, 1, NULL);

    GstElement * nvvidconv1, * nvvidconv2;
    GstCapsFeatures * feature = NULL;

    nvvidconv1 = gst_element_factory_make("videoconvert", "nvvidconv1");
    if (!nvvidconv1) {
      g_printerr("Failed to create 'nvvidcovn1'\n");
      return NULL;
    }

    feature = gst_caps_features_new("memory:NVMM", NULL);
    gst_caps_set_features(caps, 0, feature);
    g_object_set(G_OBJECT(cap_filter), "caps", caps, NULL);

    nvvidconv2 = gst_element_factory_make("capsfilter", "nvvidconv2");
    if (!nvvidconv2) {
      g_printerr("Failed to create 'nvvidcovn2'\n");
      return NULL;
    }

    g_object_set(G_OBJECT(nvvidconv2), "gpu-id", 0, NULL);

    gst_bin_add_many(GST_BIN(bin), src_elem, cap_filter, nvvidconv1, nvvidconv2, cap_filter, NULL);

    //NVGSTDS_LINK_ELEMENT (src_elem, nvvidconv1);
    //NVGSTDS_LINK_ELEMENT (nvvidconv1, nvvidconv2);
    //NVGSTDS_LINK_ELEMENT (nvvidconv2, cap_filter);
    if (!gst_element_link(src_elem, nvvidconv1)) {
      g_printerr("Failed to link 'src_elem, nvvidcovn1'\n");
      return NULL;
    }
    if (!gst_element_link(nvvidconv1, nvvidconv2)) {
      g_printerr("Failed to link 'nvvidcovn1, nvvidcovn2'\n");
      return NULL;
    }
    if (!gst_element_link(nvvidconv2, cap_filter)) {
      g_printerr("Failed to link 'nvvidcovn2, cap_filter'\n");
      return NULL;
    }

    // NVGSTDS_BIN_ADD_GHOST_PAD (bin, cap_filter, "src");
    GstPad * gstpad = gst_element_get_static_pad(cap_filter, "src");
    gst_element_add_pad(bin, gst_ghost_pad_new("src", gstpad));
    gst_object_unref(gstpad);

    gchar device[64];
    g_snprintf(device, sizeof(device), "/dev/video%d", 0);
    g_object_set(G_OBJECT(src_elem), "device", device, NULL);

    return bin;
  }

//#############################################################################

/* Square root calculation */
double SqrtNumber(double num) {
  double lower_bound = 0;
  double upper_bound = num;
  double temp = 0; /* ek edited this line */

  int nCount = 50;

  while (nCount != 0) {
    temp = (lower_bound + upper_bound) / 2;
    if (temp * temp == num) {
      return temp;
    } else if (temp * temp > num)

    {
      upper_bound = temp;
    } else {
      lower_bound = temp;
    }
    nCount--;
  }
  return temp;
}
//#############################################################################

/* Function necessary to create the labels of a new table in the scene */

NvDsDisplayMeta * CreaTavolo(NvDsDisplayMeta * custom_display_metaNEW_TABLE, int x, int y, int nr) {
  int offset = 0;
  NvOSD_TextParams * custom_txt_paramsNEW_TABLE = & custom_display_metaNEW_TABLE -> text_params[0];
  custom_display_metaNEW_TABLE -> num_labels = 1;
  custom_txt_paramsNEW_TABLE -> display_text = g_malloc0(MAX_DISPLAY_LEN);
  offset = snprintf(custom_txt_paramsNEW_TABLE -> display_text, MAX_DISPLAY_LEN, "TAVOLO %d", nr);
  custom_txt_paramsNEW_TABLE -> x_offset = x;
  custom_txt_paramsNEW_TABLE -> y_offset = y;

  custom_txt_paramsNEW_TABLE -> font_params.font_name = "Serif";
  custom_txt_paramsNEW_TABLE -> font_params.font_size = 12;
  custom_txt_paramsNEW_TABLE -> font_params.font_color.red = 1.0;
  custom_txt_paramsNEW_TABLE -> font_params.font_color.green = 1.0;
  custom_txt_paramsNEW_TABLE -> font_params.font_color.blue = 1.0;
  custom_txt_paramsNEW_TABLE -> font_params.font_color.alpha = 1.0;

  custom_txt_paramsNEW_TABLE -> set_bg_clr = 1;
  custom_txt_paramsNEW_TABLE -> text_bg_clr.red = 0.0;
  custom_txt_paramsNEW_TABLE -> text_bg_clr.green = 0.0;
  custom_txt_paramsNEW_TABLE -> text_bg_clr.blue = 0.0;
  custom_txt_paramsNEW_TABLE -> text_bg_clr.alpha = 1.0;

  return custom_display_metaNEW_TABLE;
}

//#############################################################################

/* This is the buffer probe function that we have registered on the sink pad
 * of the OSD element. All the infer elements in the pipeline shall attach
 * their metadata to the GstBuffer, here we will iterate & process the metadata
 * forex: class ids to strings, counting of class_id objects etc. */

static GstPadProbeReturn
osd_sink_pad_buffer_probe(GstPad * pad, GstPadProbeInfo * info,
  gpointer u_data) {

  struct Sala sala;
  GstBuffer * buf = (GstBuffer * ) info -> data;
  guint num_rects = 0;

  //Distance calculation prototipe
  NvDsObjectMeta * obj_meta = NULL;
  NvDsObjectMeta * obj_meta2 = NULL;

  guint person_count = 0;
  NvDsMetaList * l_frame = NULL;
  NvDsMetaList * l_obj = NULL;
  NvDsDisplayMeta * display_meta = NULL;
  NvDsDisplayMeta * custom_display_metaNEW_TABLE = NULL;
  NvOSD_TextParams * custom_txt_paramsNEW_TABLE = NULL;
  float x = 0.0;
  float y = 0.0;

  NvDsBatchMeta * batch_meta = gst_buffer_get_nvds_batch_meta(buf);

  for (l_frame = batch_meta -> frame_meta_list; l_frame != NULL; l_frame = l_frame -> next) {
    NvDsFrameMeta * frame_meta = (NvDsFrameMeta * )(l_frame -> data);
    int offset = 0;

    for (l_obj = frame_meta -> obj_meta_list; l_obj != NULL; l_obj = l_obj -> next) {
      obj_meta = (NvDsObjectMeta * )(l_obj -> data);
      if (l_obj -> next != NULL) {
        obj_meta2 = (NvDsObjectMeta * )(l_obj -> next -> data);
        x = (obj_meta -> rect_params).top / 2 - (obj_meta2 -> rect_params).top / 2;
        y = (obj_meta -> rect_params).left / 2 - (obj_meta2 -> rect_params).left / 2;
        g_print("\nDistance = %f ", SqrtNumber(x * x + y * y));

        //########### PRINT ALARM IF EXCESSIVE CLOSURE DETECTED ####################
        if (SqrtNumber(x * x + y * y) < 180) {
          NvDsDisplayMeta * custom_display_meta = NULL;
          custom_display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
          NvOSD_TextParams * custom_txt_params = & custom_display_meta -> text_params[0];
          custom_display_meta -> num_labels = 1;
          custom_txt_params -> display_text = g_malloc0(MAX_DISPLAY_LEN);
          offset = snprintf(custom_txt_params -> display_text, MAX_DISPLAY_LEN, "Distance Alarm 	%f", SqrtNumber(x * x + y * y));
          custom_txt_params -> x_offset = 800;
          custom_txt_params -> y_offset = 10;

          custom_txt_params -> font_params.font_name = "Serif";
          custom_txt_params -> font_params.font_size = 30;
          custom_txt_params -> font_params.font_color.red = 1.0;
          custom_txt_params -> font_params.font_color.green = 1.0;
          custom_txt_params -> font_params.font_color.blue = 1.0;
          custom_txt_params -> font_params.font_color.alpha = 1.0;

          custom_txt_params -> set_bg_clr = 1;
          custom_txt_params -> text_bg_clr.red = 0.0;
          custom_txt_params -> text_bg_clr.green = 0.0;
          custom_txt_params -> text_bg_clr.blue = 0.0;
          custom_txt_params -> text_bg_clr.alpha = 1.0;
          nvds_add_display_meta_to_frame(frame_meta, custom_display_meta);
        }
        //################################################################################
      }

      //######## TERMINAL PRINT OF THE COORDINATES OF EVERY PERSON IN THE SCENE #########
      if (obj_meta -> class_id == PGIE_CLASS_ID_PERSON) {
        person_count++;
        num_rects++;

        g_print("\n########### Peerson %ld ###########\n", obj_meta -> object_id);
        g_print("#  (%f , %f)\t#\n", (obj_meta -> rect_params).top / 2, (obj_meta -> rect_params).left / 2);

        g_print("#################################\n");
      }
      //#################################################################################

    }
    g_print("_________________________________________\n");

    //########## PRINT THE NUMBER OF PEOPLE IN THE SCENE ######################
    display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
    NvOSD_TextParams * txt_params = & display_meta -> text_params[0];
    display_meta -> num_labels = 1;
    txt_params -> display_text = g_malloc0(MAX_DISPLAY_LEN);
    offset = snprintf(txt_params -> display_text, MAX_DISPLAY_LEN, "Numero Persone = %i ", person_count);
    /* Now set the offsets where the string should appear */
    txt_params -> x_offset = 10;
    txt_params -> y_offset = 12;

    /* Font , font-color and font-size */
    txt_params -> font_params.font_name = "Comic Sans";
    txt_params -> font_params.font_size = 20;
    txt_params -> font_params.font_color.red = 1.0;
    txt_params -> font_params.font_color.green = 1.0;
    txt_params -> font_params.font_color.blue = 1.0;
    txt_params -> font_params.font_color.alpha = 1.0;

    /* Text background color */
    txt_params -> set_bg_clr = 1;
    txt_params -> text_bg_clr.red = 0.0;
    txt_params -> text_bg_clr.green = 0.0;
    txt_params -> text_bg_clr.blue = 0.0;
    txt_params -> text_bg_clr.alpha = 1.0;

    nvds_add_display_meta_to_frame(frame_meta, display_meta);
    //########################################################################

    //################## PRINT THE NUMBER OF THE TABLE ###########################
    /* The CreaTavolo function accepts the pointer to the current frame,
the (x, y) coordinates of the table in the scene and the table number */
    if (LIVECAM = 0) {
      nvds_add_display_meta_to_frame(frame_meta, CreaTavolo(nvds_acquire_display_meta_from_pool(batch_meta), 880, 680, 1)); // aggiungo tavolo 1	

      nvds_add_display_meta_to_frame(frame_meta, CreaTavolo(nvds_acquire_display_meta_from_pool(batch_meta), 1170, 635, 2)); //aggiungo tavolo 2

      nvds_add_display_meta_to_frame(frame_meta, CreaTavolo(nvds_acquire_display_meta_from_pool(batch_meta), 1615, 740, 3)); //aggiungo tavolo 3
      //########################################################################
    }
  }

  g_print("Frame Number = %d", frame_number);
  frame_number++;
  return GST_PAD_PROBE_OK;
}

static gboolean
bus_call(GstBus * bus, GstMessage * msg, gpointer data) {
  GMainLoop * loop = (GMainLoop * ) data;
  switch (GST_MESSAGE_TYPE(msg)) {
  case GST_MESSAGE_EOS:
    g_print("End of stream\n");
    g_main_loop_quit(loop);
    break;
  case GST_MESSAGE_ERROR: {
    gchar * debug;
    GError * error;
    gst_message_parse_error(msg, & error, & debug);
    g_printerr("ERROR from element %s: %s\n",
      GST_OBJECT_NAME(msg -> src), error -> message);
    if (debug)
      g_printerr("Error details: %s\n", debug);
    g_free(debug);
    g_error_free(error);
    g_main_loop_quit(loop);
    break;
  }
  default:
    break;
  }
  return TRUE;
}

/* Tracker config parsing */

#define CHECK_ERROR(error) \
    if (error) { \
        g_printerr ("Error while parsing config file: %s\n", error->message); \
        goto done; \
    }

#define CONFIG_GROUP_TRACKER "tracker"
#define CONFIG_GROUP_TRACKER_WIDTH "tracker-width"
#define CONFIG_GROUP_TRACKER_HEIGHT "tracker-height"
#define CONFIG_GROUP_TRACKER_LL_CONFIG_FILE "ll-config-file"
#define CONFIG_GROUP_TRACKER_LL_LIB_FILE "ll-lib-file"
#define CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS "enable-batch-process"
#define CONFIG_GPU_ID "gpu-id"

static gchar *
  get_absolute_file_path(gchar * cfg_file_path, gchar * file_path) {
    gchar abs_cfg_path[PATH_MAX + 1];
    gchar * abs_file_path;
    gchar * delim;

    if (file_path && file_path[0] == '/') {
      return file_path;
    }

    if (!realpath(cfg_file_path, abs_cfg_path)) {
      g_free(file_path);
      return NULL;
    }

    // Return absolute path of config file if file_path is NULL.
    if (!file_path) {
      abs_file_path = g_strdup(abs_cfg_path);
      return abs_file_path;
    }

    delim = g_strrstr(abs_cfg_path, "/");
    *(delim + 1) = '\0';

    abs_file_path = g_strconcat(abs_cfg_path, file_path, NULL);
    g_free(file_path);

    return abs_file_path;
  }

static gboolean
set_tracker_properties(GstElement * nvtracker) {
  gboolean ret = FALSE;
  GError * error = NULL;
  gchar ** keys = NULL;
  gchar ** key = NULL;
  GKeyFile * key_file = g_key_file_new();

  if (!g_key_file_load_from_file(key_file, TRACKER_CONFIG_FILE, G_KEY_FILE_NONE, &
      error)) {
    g_printerr("Failed to load config file: %s\n", error -> message);
    return FALSE;
  }

  keys = g_key_file_get_keys(key_file, CONFIG_GROUP_TRACKER, NULL, & error);
  CHECK_ERROR(error);

  for (key = keys;* key; key++) {
    if (!g_strcmp0( * key, CONFIG_GROUP_TRACKER_WIDTH)) {
      gint width =
        g_key_file_get_integer(key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_WIDTH, & error);
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "tracker-width", width, NULL);
    } else if (!g_strcmp0( * key, CONFIG_GROUP_TRACKER_HEIGHT)) {
      gint height =
        g_key_file_get_integer(key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_HEIGHT, & error);
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "tracker-height", height, NULL);
    } else if (!g_strcmp0( * key, CONFIG_GPU_ID)) {
      guint gpu_id =
        g_key_file_get_integer(key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GPU_ID, & error);
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "gpu_id", gpu_id, NULL);
    } else if (!g_strcmp0( * key, CONFIG_GROUP_TRACKER_LL_CONFIG_FILE)) {
      char * ll_config_file = get_absolute_file_path(TRACKER_CONFIG_FILE,
        g_key_file_get_string(key_file,
          CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_LL_CONFIG_FILE, & error));
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "ll-config-file", ll_config_file, NULL);
    } else if (!g_strcmp0( * key, CONFIG_GROUP_TRACKER_LL_LIB_FILE)) {
      char * ll_lib_file = get_absolute_file_path(TRACKER_CONFIG_FILE,
        g_key_file_get_string(key_file,
          CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_LL_LIB_FILE, & error));
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "ll-lib-file", ll_lib_file, NULL);
    } else if (!g_strcmp0( * key, CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS)) {
      gboolean enable_batch_process =
        g_key_file_get_integer(key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS, & error);
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "enable_batch_process",
        enable_batch_process, NULL);
    } else {
      g_printerr("Unknown key '%s' for group [%s]", * key,
        CONFIG_GROUP_TRACKER);
    }
  }

  ret = TRUE;
  done:
    if (error) {
      g_error_free(error);
    }
  if (keys) {
    g_strfreev(keys);
  }
  if (!ret) {
    g_printerr("%s failed", __func__);
  }
  return ret;
}

/* Main function */

int
main(int argc, char * argv[]) {
  if ( * argv[2] == '0') LIVECAM = 0;
  if ( * argv[2] == '1') LIVECAM = 1;

  /* I enter the file-source branch */
  if (LIVECAM == 0) {
	    g_printerr("LIVECAM DISATTIVA\n\n");
	    GMainLoop * loop = NULL;
	    GstElement * pipeline = NULL, * source = NULL, * h264parser = NULL,
	      * decoder = NULL, * streammux = NULL, * sink = NULL, * pgie = NULL, * nvvidconv = NULL,
	      * nvosd = NULL, * nvtracker = NULL;

	    #ifdef PLATFORM_TEGRA
	    GstElement * transform = NULL;
	    #endif
	    GstBus * bus = NULL;
	    guint bus_watch_id = 0;
	    GstPad * osd_sink_pad = NULL;

	    /* Check input arguments */
	    if (argc != 3) {
	      g_printerr("Usage: %s <elementary H264 filename> 0/1\n", argv[0]);
	      return -1;
	    }

	    /* Standard GStreamer initialization */
	    gst_init( & argc, & argv);
	    loop = g_main_loop_new(NULL, FALSE);

	    /* Create gstreamer elements */

	    /* Create Pipeline element that will be a container of other elements */
	    pipeline = gst_pipeline_new("PeopleTrackPipeline");

	    /* Source element for reading from the file */
	    source = gst_element_factory_make("filesrc", "file-source");

	    /* Since the data format in the input file is elementary h264 stream,
	     * we need a h264parser */
	    h264parser = gst_element_factory_make("h264parse", "h264-parser");

	    /* Use nvdec_h264 for hardware accelerated decode on GPU */
	    decoder = gst_element_factory_make("nvv4l2decoder", "nvv4l2-decoder");

	    /* Create nvstreammux instance to form batches from one or more sources. */
	    streammux = gst_element_factory_make("nvstreammux", "stream-muxer");

	    if (!pipeline || !streammux) {
	      g_printerr("One element could not be created. Exiting.\n");
	      return -1;
	    }

	    /* Use nvinfer to run inferencing on decoder's output,
	     * behaviour of inferencing is set through config file */
	    pgie = gst_element_factory_make("nvinfer", "primary-nvinference-engine");

	    /* We need to have a tracker to track the identified objects */

	    nvtracker = gst_element_factory_make("nvtracker", "tracker");
	    g_print("\nThe tracker has been successfully initialized and is active\n");

	    /* Use convertor to convert from NV12 to RGBA as required by nvosd */
	    nvvidconv = gst_element_factory_make("nvvideoconvert", "nvvideo-converter");

	    /* Create OSD to draw on the converted RGBA buffer */
	    nvosd = gst_element_factory_make("nvdsosd", "nv-onscreendisplay");

	    /* Finally render the osd output */
	    #ifdef PLATFORM_TEGRA
	    transform = gst_element_factory_make("nvegltransform", "nvegl-transform");
	    #endif
	    sink = gst_element_factory_make("nveglglessink", "nvvideo-renderer");

	    if (!source || !h264parser || !decoder || !pgie ||
	      !nvtracker || !nvvidconv || !nvosd || !sink) {
	      g_printerr("One element could not be created. Exiting.\n");
	      return -1;
	    }

	    #ifdef PLATFORM_TEGRA
	    if (!transform) {
	      g_printerr("One tegra element could not be created. Exiting.\n");
	      return -1;
	    }
	    #endif

	    /* Set the input filename to the source element */
	    g_object_set(G_OBJECT(source), "location", argv[1], NULL);

	    g_object_set(G_OBJECT(streammux), "batch-size", 1, NULL);

	    g_object_set(G_OBJECT(streammux), "width", MUXER_OUTPUT_WIDTH, "height",
	      MUXER_OUTPUT_HEIGHT,
	      "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, NULL);

	    /* Set all the necessary properties of the nvinfer element,
	     * the necessary ones are : */
	    g_object_set(G_OBJECT(pgie), "config-file-path", PGIE_CONFIG_FILE, NULL);

	    /* Set necessary properties of the tracker element. */
	    if (!set_tracker_properties(nvtracker)) {
	      g_printerr("Failed to set tracker properties. Exiting.\n");
	      return -1;
	    }

	    /* we add a message handler */
	    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
	    gst_object_unref(bus);

	    /* Set up the pipeline */
	    /* we add all elements into the pipeline */
	    /* decoder | pgie1 | nvtracker| etc.. */
	    #ifdef PLATFORM_TEGRA
	    gst_bin_add_many(GST_BIN(pipeline),
	      source, h264parser, decoder, streammux, pgie, nvtracker,
	      nvvidconv, nvosd, transform, sink, NULL);
	    #else
	    gst_bin_add_many(GST_BIN(pipeline),
	      source, h264parser, decoder, streammux, pgie, nvtracker,
	      nvvidconv, nvosd, sink, NULL);
	    #endif

	    GstPad * sinkpad, * srcpad;
	    gchar pad_name_sink[16] = "sink_0";
	    gchar pad_name_src[16] = "src";

	    sinkpad = gst_element_get_request_pad(streammux, pad_name_sink);
	    if (!sinkpad) {
	      g_printerr("Streammux request sink pad failed. Exiting.\n");
	      return -1;
	    }

	    srcpad = gst_element_get_static_pad(decoder, pad_name_src);
	    if (!srcpad) {
	      g_printerr("Decoder request src pad failed. Exiting.\n");
	      return -1;
	    }

	    if (gst_pad_link(srcpad, sinkpad) != GST_PAD_LINK_OK) {
	      g_printerr("Failed to link decoder to stream muxer. Exiting.\n");
	      return -1;
	    }

	    gst_object_unref(sinkpad);
	    gst_object_unref(srcpad);

	    /* Link the elements together */
	    if (!gst_element_link_many(source, h264parser, decoder, NULL)) {
	      g_printerr("Elements could not be linked: 1. Exiting.\n");
	      return -1;
	    }

	    #ifdef PLATFORM_TEGRA
	    if (!gst_element_link_many(streammux, pgie, nvtracker,

		nvvidconv, nvosd, transform, sink, NULL)) {
	      g_printerr("Elements could not be linked. Exiting.\n");
	      return -1;
	    }
	    #else
	    if (!gst_element_link_many(streammux, pgie, nvtracker,

		nvvidconv, nvosd, sink, NULL)) {
	      g_printerr("Elements could not be linked. Exiting.\n");
	      return -1;
	    }
	    #endif

	    /* Lets add probe to get informed of the meta data generated, we add probe to
	     * the sink pad of the osd element, since by that time, the buffer would have
	     * had got all the metadata. */
	    osd_sink_pad = gst_element_get_static_pad(nvosd, "sink");
	    if (!osd_sink_pad)
	      g_print("Unable to get sink pad\n");
	    else
	      gst_pad_add_probe(osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
		osd_sink_pad_buffer_probe, NULL, NULL);

	    /* Set the pipeline to "playing" state */
	    g_print("Now playing: %s\n", argv[1]);
	    gst_element_set_state(pipeline, GST_STATE_PLAYING);

	    /* Iterate */
	    g_print("Running...\n");
	    g_main_loop_run(loop);

	    /* Out of the main loop, clean up nicely */
	    g_print("Returned, stopping playback\n");
	    gst_element_set_state(pipeline, GST_STATE_NULL);
	    g_print("Deleting pipeline\n");
	    gst_object_unref(GST_OBJECT(pipeline));
	    g_source_remove(bus_watch_id);
	    g_main_loop_unref(loop);
  } else {
    
    /* I enter the livecam branch */
    g_printerr("LIVECAM ATTIVA\n\n");
    GMainLoop * loop = NULL;
    GstElement * pipeline = NULL, * streammux = NULL, * sink = NULL, * pgie = NULL, * nvvidconv = NULL,
      * nvosd = NULL, * nvtracker = NULL;

    #ifdef PLATFORM_TEGRA
    GstElement * transform = NULL;
    #endif
    GstBus * bus = NULL;
    guint bus_watch_id = 0;
    GstPad * osd_sink_pad = NULL;

    /* Check input arguments */
    if (argc != 3) {
      g_printerr("Usage: %s <elementary H264 filename> 0/1\n", argv[0]);
      return -1;
    }

    /* Standard GStreamer initialization */
    gst_init( & argc, & argv);
    loop = g_main_loop_new(NULL, FALSE);
    pipeline = gst_pipeline_new("PeopleTrackPipeline");
    streammux = gst_element_factory_make("nvstreammux", "stream-muxer");

    if (!pipeline || !streammux) {
      g_printerr("One element could not be created. Exiting1.\n");
      return -1;
    }

    gst_bin_add(GST_BIN(pipeline), streammux);

    GstPad * sinkpad, * srcpad;
    gchar pad_name[16] = {};
    GstElement * source_bin = create_camera_source_bin(0, argv[1]);

    if (!source_bin) {
      g_printerr("Failed to create source bin. Exiting2.\n");
      return -1;
    }

    gst_bin_add(GST_BIN(pipeline), source_bin);

    g_snprintf(pad_name, 15, "sink_%u", 1);
    sinkpad = gst_element_get_request_pad(streammux, pad_name);
    if (!sinkpad) {
      g_printerr("Streammux request sink pad failed. Exiting3.\n");
      return -1;
    }

    srcpad = gst_element_get_static_pad(source_bin, "src");
    if (!srcpad) {
      g_printerr("Failed to get src pad of source bin. Exiting4.\n");
      return -1;
    }

    if (gst_pad_link(srcpad, sinkpad) != GST_PAD_LINK_OK) {
      g_printerr("Failed to link source bin to stream muxer. Exiting5.\n");
      return -1;
    }

    gst_object_unref(srcpad);
    gst_object_unref(sinkpad);

    /* Use nvinfer to run inferencing on decoder's output,
     * behaviour of inferencing is set through config file */
    pgie = gst_element_factory_make("nvinfer", "primary-nvinference-engine");

    /* We need to have a tracker to track the identified objects */

    nvtracker = gst_element_factory_make("nvtracker", "tracker");
    g_print("\nThe tracker has been successfully initialized and is active\n");
    /* Use convertor to convert from NV12 to RGBA as required by nvosd */
    nvvidconv = gst_element_factory_make("nvvideoconvert", "nvvideo-converter");

    /* Create OSD to draw on the converted RGBA buffer */
    nvosd = gst_element_factory_make("nvdsosd", "nv-onscreendisplay");

    /* Finally render the osd output */
    #ifdef PLATFORM_TEGRA
    transform = gst_element_factory_make("nvegltransform", "nvegl-transform");
    #endif
    sink = gst_element_factory_make("nveglglessink", "nvvideo-renderer");

    if (!pgie ||
      !nvtracker || !nvvidconv || !nvosd || !sink) {
      g_printerr("One element could not be created. Exiting.6\n");
      return -1;
    }

    #ifdef PLATFORM_TEGRA
    if (!transform) {
      g_printerr("One tegra element could not be created. Exiting.7\n");
      return -1;
    }
    #endif

    g_object_set(G_OBJECT(streammux), "batch-size", 1, NULL);

    g_object_set(G_OBJECT(streammux), "width", MUXER_OUTPUT_WIDTH, "height",
      MUXER_OUTPUT_HEIGHT,
      "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, NULL);

    /* Set all the necessary properties of the nvinfer element,
     * the necessary ones are : */
    g_object_set(G_OBJECT(pgie), "config-file-path", PGIE_CONFIG_FILE, NULL);

    /* Set necessary properties of the tracker element. */
    if (!set_tracker_properties(nvtracker)) {
      g_printerr("Failed to set tracker properties. Exiting.\n");
      return -1;
    }

    g_object_set(G_OBJECT(sink), "sync", FALSE, NULL);

    /* we add a message handler */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    /* Set up the pipeline */
    /* we add all elements into the pipeline */
    /* decoder | pgie1 | nvtracker| etc.. */
    #ifdef PLATFORM_TEGRA
    gst_bin_add_many(GST_BIN(pipeline),
      pgie, nvtracker,
      nvvidconv, nvosd, transform, sink, NULL);
    #else
    gst_bin_add_many(GST_BIN(pipeline),
      pgie, nvtracker,
      nvvidconv, nvosd, sink, NULL);
    #endif

    #ifdef PLATFORM_TEGRA
    if (!gst_element_link_many(streammux, pgie, nvtracker,

        nvvidconv, nvosd, transform, sink, NULL)) {
      g_printerr("Elements could not be linked. Exiting.\n");
      return -1;
    }
    #else
    if (!gst_element_link_many(streammux, pgie, nvtracker,

        nvvidconv, nvosd, sink, NULL)) {
      g_printerr("Elements could not be linked. Exiting.\n");
      return -1;
    }
    #endif

    /* Lets add probe to get informed of the meta data generated, we add probe to
     * the sink pad of the osd element, since by that time, the buffer would have
     * had got all the metadata. */
    osd_sink_pad = gst_element_get_static_pad(nvosd, "sink");
    if (!osd_sink_pad)
      g_print("Unable to get sink pad\n");
    else
      gst_pad_add_probe(osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
        osd_sink_pad_buffer_probe, NULL, NULL);

    /* Set the pipeline to "playing" state */
    g_print("Now playing: %s\n", argv[1]);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Iterate */
    g_print("Running...\n");
    g_main_loop_run(loop);

    /* Out of the main loop, clean up nicely */
    g_print("Returned, stopping playback\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_print("Deleting pipeline\n");
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
  }
  return 0;
}
