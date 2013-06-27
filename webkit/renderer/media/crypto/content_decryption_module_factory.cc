// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/renderer/media/crypto/content_decryption_module_factory.h"

#include "base/logging.h"
#include "media/crypto/aes_decryptor.h"
#include "webkit/renderer/media/crypto/key_systems.h"

#if defined(ENABLE_PEPPER_CDMS)
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebMediaPlayerClient.h"
#include "webkit/plugins/ppapi/ppapi_plugin_instance.h"
#include "webkit/plugins/ppapi/ppapi_webplugin_impl.h"
#include "webkit/renderer/media/crypto/ppapi_decryptor.h"
#endif  // defined(ENABLE_PEPPER_CDMS)

namespace webkit_media {

#if defined(ENABLE_PEPPER_CDMS)
// Returns the PluginInstance associated with the Helper Plugin.
// If a non-NULL pointer is returned, the caller must call closeHelperPlugin()
// when the Helper Plugin is no longer needed.
static scoped_refptr<webkit::ppapi::PluginInstance> CreateHelperPlugin(
    const std::string& plugin_type,
    WebKit::WebMediaPlayerClient* web_media_player_client,
    WebKit::WebFrame* web_frame) {
  DCHECK(web_media_player_client);
  DCHECK(web_frame);

  WebKit::WebPlugin* web_plugin = web_media_player_client->createHelperPlugin(
      WebKit::WebString::fromUTF8(plugin_type), web_frame);
  if (!web_plugin)
    return NULL;

  DCHECK(!web_plugin->isPlaceholder());  // Prevented by Blink.
  // Only Pepper plugins are supported, so it must be a ppapi object.
  webkit::ppapi::WebPluginImpl* ppapi_plugin =
      static_cast<webkit::ppapi::WebPluginImpl*>(web_plugin);
  return ppapi_plugin->instance();
}

static scoped_ptr<media::MediaKeys> CreatePpapiDecryptor(
    const std::string& key_system,
    const media::KeyAddedCB& key_added_cb,
    const media::KeyErrorCB& key_error_cb,
    const media::KeyMessageCB& key_message_cb,
    const base::Closure& destroy_plugin_cb,
    WebKit::WebMediaPlayerClient* web_media_player_client,
    WebKit::WebFrame* web_frame) {
  DCHECK(web_media_player_client);
  DCHECK(web_frame);

  std::string plugin_type = GetPepperType(key_system);
  DCHECK(!plugin_type.empty());
  const scoped_refptr<webkit::ppapi::PluginInstance>& plugin_instance =
      CreateHelperPlugin(plugin_type, web_media_player_client, web_frame);
  if (!plugin_instance.get()) {
    DLOG(ERROR) << "ProxyDecryptor: plugin instance creation failed.";
    return scoped_ptr<media::MediaKeys>();
  }

  scoped_ptr<PpapiDecryptor> decryptor =
      PpapiDecryptor::Create(key_system,
                             plugin_instance,
                             key_added_cb,
                             key_error_cb,
                             key_message_cb,
                             destroy_plugin_cb);

  if (!decryptor) {
    ContentDecryptionModuleFactory::DestroyHelperPlugin(
        web_media_player_client);
  }
  // Else the new object will call destroy_plugin_cb to destroy Helper Plugin.

  return scoped_ptr<media::MediaKeys>(decryptor.Pass());
}

void ContentDecryptionModuleFactory::DestroyHelperPlugin(
    WebKit::WebMediaPlayerClient* web_media_player_client) {
  web_media_player_client->closeHelperPlugin();
}
#endif  // defined(ENABLE_PEPPER_CDMS)

scoped_ptr<media::MediaKeys> ContentDecryptionModuleFactory::Create(
    const std::string& key_system,
#if defined(ENABLE_PEPPER_CDMS)
    WebKit::WebMediaPlayerClient* web_media_player_client,
    WebKit::WebFrame* web_frame,
    const base::Closure& destroy_plugin_cb,
#elif defined(OS_ANDROID)
    scoped_ptr<media::MediaKeys> media_keys,
#endif  // defined(ENABLE_PEPPER_CDMS)
    const media::KeyAddedCB& key_added_cb,
    const media::KeyErrorCB& key_error_cb,
    const media::KeyMessageCB& key_message_cb) {
  if (CanUseAesDecryptor(key_system)) {
    return scoped_ptr<media::MediaKeys>(
        new media::AesDecryptor(key_added_cb, key_error_cb, key_message_cb));
  }

#if defined(ENABLE_PEPPER_CDMS)
  // TODO(ddorwin): Remove when the WD API implementation supports loading
  // Pepper-based CDMs: http://crbug.com/250049
  if (!web_media_player_client)
    return scoped_ptr<media::MediaKeys>();

  return CreatePpapiDecryptor(
      key_system, key_added_cb, key_error_cb, key_message_cb,
      destroy_plugin_cb, web_media_player_client, web_frame);
#elif defined(OS_ANDROID)
  return media_keys.Pass();
#else
  return scoped_ptr<media::MediaKeys>();
#endif  // defined(ENABLE_PEPPER_CDMS)
}

}  // namespace webkit_media
