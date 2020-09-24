/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <dlfcn.h>
#include "deepstream_c2d_msg.h"
#include "gst-nvdssr.h"


static void
nvds_msgapi_connect_callback (NvDsMsgApiHandle h_ptr, NvDsMsgApiEventType ds_evt)
{

}

static void
subscribe_cb (NvDsMsgApiErrorType flag, void *msg, int msg_len, char *topic,
              void *uData)
{
  if(flag == NVDS_MSGAPI_ERR) {
    g_print ("Error in consuming message from kafka broker\n");
  } else {
    g_print ("Consuming message, on topic[%s]. Payload =%.*s\n\n", topic,
             msg_len, (char *) msg);
  }
}

NvDsC2DContext*
start_cloud_to_device_messaging (NvDsMsgConsumerConfig *config, void *uData)
{
  gchar *error = NULL;
  NvDsC2DContext *ctx = NULL;
  gchar **topicList = NULL;
  gint i, numTopic;

  if (!config->conn_str || !config->proto_lib || !config->topicList) {
    g_print ("%s: NULL parameters\n", __func__);
    return NULL;
  }

  ctx = g_new0 (NvDsC2DContext, 1);
  ctx->protoLib = g_strdup (config->proto_lib);
  ctx->connStr = g_strdup (config->conn_str);
  ctx->configFile = g_strdup (config->config_file_path);

  ctx->libHandle = dlopen(ctx->protoLib, RTLD_LAZY);
  if (!ctx->libHandle) {
    g_print ("unable to open shared library\n");
    goto error;
  }

  dlerror();    /* Clear any existing error */

  ctx->nvds_msgapi_connect = (nvds_msgapi_connect_ptr) dlsym (ctx->libHandle, "nvds_msgapi_connect");
  ctx->nvds_msgapi_disconnect = (nvds_msgapi_disconnect_ptr) dlsym (ctx->libHandle, "nvds_msgapi_disconnect");
  ctx->nvds_msgapi_subscribe = (nvds_msgapi_subscribe_ptr) dlsym (ctx->libHandle, "nvds_msgapi_subscribe");
  if ((error = dlerror()) != NULL) {
    g_print ("%s\n", error);
    goto error;
  }

  ctx->connHandle = ctx->nvds_msgapi_connect (ctx->connStr,
                    (nvds_msgapi_connect_cb_t) nvds_msgapi_connect_callback,
                     ctx->configFile);
  if (!ctx->connHandle) {
    g_print ("unable to connect to broker \n");
    goto error;
  }

  numTopic = config->topicList->len;
  topicList = g_new0 (gchar*, numTopic);
  for (i = 0; i < numTopic; i++) {
    topicList[i] = (gchar *) g_ptr_array_index (config->topicList, i);
  }

  if (ctx->nvds_msgapi_subscribe (ctx->connHandle, topicList, numTopic, subscribe_cb, (gpointer)ctx) != NVDS_MSGAPI_OK) {
    g_print ("Subscription to topic[s] failed\n");
    goto error;
  }

  g_free (topicList);
  topicList = NULL;

  return ctx;

error:
  if (ctx) {
    if (ctx->libHandle) {
      dlclose (ctx->libHandle);
      ctx->libHandle = NULL;
    }
    g_free (ctx->configFile);
    g_free (ctx->connStr);
    g_free (ctx->protoLib);
    g_free (ctx);
    ctx = NULL;
  }

  if (topicList)
    g_free (topicList);

  return ctx;
}

gboolean
stop_cloud_to_device_messaging (NvDsC2DContext *ctx)
{
  NvDsMsgApiErrorType err;
  gboolean ret = TRUE;

  g_return_val_if_fail (ctx, FALSE);

  if (ctx->nvds_msgapi_disconnect) {
    err = ctx->nvds_msgapi_disconnect (ctx->connHandle);
    if (err != NVDS_MSGAPI_OK) {
      g_print ("error(%d) in disconnect\n", err);
      ret = FALSE;
    }
    ctx->connHandle = NULL;
  }

  if (ctx->libHandle) {
    dlclose (ctx->libHandle);
    ctx->libHandle = NULL;
  }

  g_free (ctx->configFile);
  g_free (ctx->connStr);
  g_free (ctx->protoLib);
  g_free (ctx);
  return ret;
}
