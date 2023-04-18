//
// Copyright Antaris Inc 2022
//
/*
 * Use of this software is governed by the terms of the
 * Antaris SDK License Agreement entered into between Antaris
 * and you or your employer. You may only use this software as
 * permitted in that agreement. If you or your employer has not
 * entered into the Antaris SDK License Agreement, you are not
 * permitted to use this software.
 */

#ifndef __ANTARIS_SDK_ENVINRONMENT__
#define __ANTARIS_SDK_ENVINRONMENT__

#include "cJSON.h"
#define MAX_FILE_OR_PROP_LEN_NAME       256


/* refresh config by considering environment and config file */
void sdk_environment_read_config(void);
void read_config_json( cJSON ** pp_cJson);

#endif