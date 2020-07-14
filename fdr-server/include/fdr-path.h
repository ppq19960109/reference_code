
#ifndef _FDR_PATH_H_
#define _FDR_PATH_H_

#include "stdint.h"

#ifdef XM6XX

#if defined(XM650)
#define FDR_HOME_PATH   "/var/sd/app"
#elif defined(XM630)
#define FDR_HOME_PATH   "/var/sd"
#else
#error "Please define the cpu core"
#endif

#define OPLOG_PATH              FDR_HOME_PATH"/log/"
#define CAPTURE_IMAGE_PATH      FDR_HOME_PATH"/data/images/"
#define USER_IMAGE_PATH         FDR_HOME_PATH"/data/users/"
#define UPGRADE_PACKAGE_PATH    FDR_HOME_PATH"/upgrade/"
//#define DEV_PUBKEY_FILENAME     FDR_HOME_PATH"/data/ecc/dev-pubkey.ecc"
//#define DEV_PRIKEY_FILENAME     FDR_HOME_PATH"/data/ecc/dev-prikey.ecc"
//#define SVR_PUBKEY_FILENAME     FDR_HOME_PATH"/data/ecc/svr-pubkey.ecc"
#define FEATURE_SQLITE_PATH     FDR_HOME_PATH"/data/db"
#define DBS_SQLITE_FILENAME     FDR_HOME_PATH"/data/db/dbs.sqlite"
#define LOG_SQLITE_FILENAME     FDR_HOME_PATH"/data/db/log.sqlite"
#define DEFAULT_CONFIG_FILENAME FDR_HOME_PATH"/data/db/default.json"
#define UPGRADE_CONFIG_FILENAME FDR_HOME_PATH"/data/db/upgrade.json"
#define LICENSE_FILENAME        "/mnt/mtd/device.json"

#define FDR_VOICE_PATH          FDR_HOME_PATH"/data/voice"

#if defined(UI_LANG) && (UI_LANG == 1)
#define FDR_UI_PATH             FDR_HOME_PATH"/data/ui-en"
#elif defined(UI_LANG) && (UI_LANG == 2)
#define FDR_UI_PATH             FDR_HOME_PATH"/data/ui-neutral"
#elif defined(UI_LANG) && (UI_LANG == 0)
#define FDR_UI_PATH             FDR_HOME_PATH"/data/ui"
#else
#error "Please define UI_LANG to select ui lang"
#endif

//XM6XX
#else

#define FDR_HOME_PATH           "."
#define OPLOG_PATH              FDR_HOME_PATH"/log/"
#define CAPTURE_IMAGE_PATH      FDR_HOME_PATH"/images/"
#define USER_IMAGE_PATH         FDR_HOME_PATH"/users/"
#define UPGRADE_PACKAGE_PATH    FDR_HOME_PATH"/upgrade/"
//#define DEV_PUBKEY_FILENAME     FDR_HOME_PATH"/pubkey.ecc"
//#define DEV_PRIKEY_FILENAME     FDR_HOME_PATH"/prikey.ecc"
//#define SVR_PUBKEY_FILENAME     FDR_HOME_PATH"/pubkey.ecc"
#define FEATURE_SQLITE_PATH     FDR_HOME_PATH"/"
#define DBS_SQLITE_FILENAME     FDR_HOME_PATH"/dbs.sqlite"
#define LOG_SQLITE_FILENAME     FDR_HOME_PATH"/log.sqlite"
#define DEFAULT_CONFIG_FILENAME FDR_HOME_PATH"/default.json"
#define FDR_UI_PATH             FDR_HOME_PATH"/ui"
#define LICENSE_FILENAME        FDR_HOME_PATH"/device.json"

#define FDR_VOICE_PATH          FDR_HOME_PATH"/voice"

#endif //XM6XX

#endif

