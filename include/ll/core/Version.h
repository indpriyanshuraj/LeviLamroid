#pragma once

#define LL_LABEL_ALPHA 0
#define LL_LABEL_BETA  1
#define LL_LABEL_NONE  255

#define LL_VERSION_MAJOR         1
#define LL_VERSION_MINOR         0
#define LL_VERSION_PATCH         5
#define LL_VERSION_COMMIT_SHA    "android"

#define LL_WORKSPACE_FOLDER "/sdcard/Android/media/org.levimc.launcher/minecraft/"

#define TARGET_BDS_PROTOCOL_VERSION 776

#define LL_VERSION_TO_STRING_INNER(ver) #ver
#define LL_VERSION_TO_STRING(ver)       LL_VERSION_TO_STRING_INNER(ver)

#define LL_FILE_VERSION_FLAG 0x00000000L
#define LL_FILE_VERSION_STRING "1.0.5+android"
