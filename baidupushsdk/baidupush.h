#pragma once


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "cyassl\include\openssl\md5.h"
#include "libcurl\include\curl\curl.h"




#define MAX_BODY 1024
#define ANDROID_APP_KEY ""
#define ANDROID_SECRECT_KEY ""
#define ANDROID_PUSHMESSAGEBODY "{\"title\":\"%s\",\"description\":\"%s\",\"notification_builder_id\": 0,\"lightapp_ctrl_keys\":{\"display_in_notification_bar\" : 0,\"enter_msg_center\":0 },\"custom_content\":{\"MAC\":\"549A11C00000\"}}"

#define IOS_APP_KEY ""
#define IOS_SECRECT_KEY ""
#define IOS_PUSHMESSAGEBODY "{\"aps\":{\"alert\":\"%s\"},\"MAC\":\"549A11C00000\"}"




int baidupush_init(void);
int baidupush_android_signle(char * channel_id,char * tittle,char * description);
int baidupush_android_all(char * tittle,char * description);
int baidupush_ios_signle(char * channel_id,char * tittle,char * description);
int baidupush_ios_all(char * tittle,char * description);
