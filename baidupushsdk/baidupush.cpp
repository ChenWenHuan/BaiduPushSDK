#include "baidupush.h"

#pragma comment(lib,"libcurl.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"cyassl.lib")

#pragma warning(disable:4996)

static BOOL UrlEncode(const char* szSrc, char* pBuf, int cbBufLen, BOOL bUpperCase)
{
	if(szSrc == NULL || pBuf == NULL || cbBufLen <= 0){
		return FALSE;
	}
	size_t len_ascii = strlen(szSrc);
	if(len_ascii == 0){
		pBuf[0] = 0;
		return TRUE;
	}
	char baseChar = bUpperCase ? 'A' : 'a';
	int cchWideChar = MultiByteToWideChar(CP_ACP, 0, szSrc, len_ascii, NULL, 0);
	LPWSTR pUnicode = (LPWSTR)malloc((cchWideChar + 1) * sizeof(WCHAR));
	if (pUnicode == NULL){
		return FALSE;
	}
	MultiByteToWideChar(CP_ACP, 0, szSrc, len_ascii, pUnicode, cchWideChar + 1);
	int cbUTF8 = WideCharToMultiByte(CP_UTF8, 0, pUnicode, cchWideChar, NULL, 0, NULL, NULL);
	LPSTR pUTF8 = (LPSTR)malloc((cbUTF8 + 1) * sizeof(CHAR));
	if(pUTF8 == NULL){
		free(pUnicode);
		return FALSE;
	}
	WideCharToMultiByte(CP_UTF8, 0, pUnicode, cchWideChar, pUTF8, cbUTF8 + 1, NULL, NULL);
	pUTF8[cbUTF8] = '\0';
	unsigned char c;
	int cbDest = 0;
	unsigned char *pSrc = (unsigned char*)pUTF8;
	unsigned char *pDest = (unsigned char*)pBuf;
	while(*pSrc && cbDest < cbBufLen - 1){
		c = *pSrc;
		if(isalpha(c) || isdigit(c) || c == '-' || c == '_' || c == '.'){
			*pDest = c;
			++pDest;
			++cbDest;
		}
		else if(c == ' '){
			*pDest = '+';
			++pDest;
			++cbDest;
		}
		else{
			if(cbDest + 3 > cbBufLen - 1){
				break;
			}
			pDest[0] = '%';
			pDest[1] = (c >= 0xA0) ? ((c >> 4) - 10 + baseChar) : ((c >> 4) + '0');
			pDest[2] = ((c & 0xF) >= 0xA)? ((c & 0xF) - 10 + baseChar) : ((c & 0xF) + '0');
			pDest += 3;
			cbDest += 3;
		}
		++pSrc;
	}
	*pDest = '\0';
	free(pUnicode);
	free(pUTF8);
	return TRUE;
}

static void bytes2Hex(char *hexString, void *bytes, int len)
{
	int i;
	unsigned char *rawBytes;
	char tmpStr[3];
	rawBytes = (unsigned char *)bytes;
	for (i = 0;i < len;i++){
		sprintf(tmpStr, "%02x", rawBytes[i]);
		memcpy(hexString + i*2,tmpStr,2);
	}
}


static void m_str2unicodestr(char * in_buf,unsigned int inlen,char * out_buf,unsigned int * out_len)
{
	unsigned int i = 0;
	if(in_buf == NULL || out_buf == NULL || inlen <= 0){
		return ;
	}
	size_t len_ascii = strlen(in_buf);
	if(len_ascii == 0 || out_len == NULL){
		out_buf[0] = 0;
		return ;
	}
	int cchWideChar = MultiByteToWideChar(CP_ACP, 0, in_buf, len_ascii, NULL, 0);
	LPWSTR pUnicode = (LPWSTR)malloc((cchWideChar + 1) * sizeof(WCHAR));
	if(pUnicode == NULL){
		return ;
	}
	cchWideChar = MultiByteToWideChar(CP_ACP, 0, in_buf, len_ascii, pUnicode, cchWideChar + 1);
	char * p_str = (char *)pUnicode;
	unsigned int ilen = 0;
	for (i = 0;i < cchWideChar;i++){
		if (p_str[2*i + 1] == 0x00){
			ilen = strlen(out_buf);
			out_buf[ilen] = p_str[2*i];
			*out_len++;
		}
		else{
			ilen = strlen(out_buf);
			out_buf[ilen] = '\\';
			out_buf[ilen + 1] = 'u';
			bytes2Hex(&out_buf[ilen + 4],(char *)&p_str[2*i],1);
			bytes2Hex(&out_buf[ilen + 2],(char *)&p_str[2*i + 1],1);
			*out_len += 6;
		}
	}
	delete pUnicode;
}




int baidupush_android_signle(char * channel_id,char * tittle,char * description)
{
	CURL * curl;
	CURLcode res;
	const char * HTTP_METHOD = "POST";
	const char * HTTP_URL = "http://api.tuisong.baidu.com/rest/3.0/push/single_device";
	const char * pAppKey = ANDROID_APP_KEY;
	const char * pSecretkey = ANDROID_SECRECT_KEY;
	char * msg = tittle;
	char msg_unicode[512] = { 0 };
	char description_unicode[512] = { 0 };
	char tmp_sign[64] = { 0 };
	char msg_buf[512] = { 0 };
	char msg_urlencode_buf[512] = { 0 };
	char pForm[1024] = { 0 };
	char pTemp[1024] = { 0 };
	char tmp_buf_src[1024] = { 0 };
	char tmp_buf_urlencode[1024] = { 0 };
	MD5_CTX ctx;
	char md5Value[128] = { 0 };
	curl = curl_easy_init();
	if ((strlen(pAppKey) == 0) || (strlen(pSecretkey) == 0)){
		fprintf(stderr, "Enter correct baidu APIKEY!\n");
		return 1;
	}
	if (curl){
		curl_easy_setopt(curl,CURLOPT_URL,HTTP_URL);
		struct curl_slist * http_headers = NULL;
		http_headers = curl_slist_append(http_headers, "Content-type: application/x-www-form-urlencoded;charset=utf-8");
		http_headers = curl_slist_append(http_headers, "User-Agent: BCCS_SDK/3.0 (Windows NT; build 7601 (Windows 7 Ultimate Edition Service Pack 1); i586) PHP/5.4.45 (Baidu Push SDK for PHP v3.0.0) cli/Unknown ZEND/2.4.0");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
		sprintf(pTemp,"apikey=%s",pAppKey);
		strcat(pForm, pTemp);
		time_t tick;
		tick = time(NULL);
		sprintf(pTemp,"&timestamp=%d",tick);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&channel_id=%s",channel_id);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&device_type=3");
		strcat(pForm, pTemp);
		unsigned int ilen = 0;
		m_str2unicodestr(msg,strlen(msg),msg_unicode,&ilen);
		m_str2unicodestr(description,strlen(description),description_unicode,&ilen);
		sprintf(msg_buf,ANDROID_PUSHMESSAGEBODY,msg_unicode,description_unicode);
		UrlEncode(msg_buf,msg_urlencode_buf,sizeof(msg_buf),TRUE);
		sprintf(pTemp,"&msg=%s",msg_urlencode_buf);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&msg_type=1");
		strcat(pForm, pTemp);
		sprintf(tmp_buf_src,"%s%sapikey=%schannel_id=%sdevice_type=3msg=%smsg_type=1timestamp=%d",HTTP_METHOD,HTTP_URL,pAppKey,channel_id,msg_buf,tick);
		strncat(tmp_buf_src,pSecretkey,strlen(pSecretkey));
		UrlEncode(tmp_buf_src,tmp_buf_urlencode,sizeof(tmp_buf_urlencode),TRUE);
		MD5_Init(&ctx);
		MD5_Update(&ctx,tmp_buf_urlencode,strlen(tmp_buf_urlencode));
		MD5_Final((unsigned char *)md5Value, &ctx);
		bytes2Hex(tmp_sign, md5Value, 16);
		sprintf(pTemp,"&sign=%s",tmp_sign);
		strcat(pForm, pTemp);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS,pForm);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
		}
		curl_slist_free_all(http_headers);
		curl_easy_cleanup(curl);
	}
	return res;
}

int baidupush_android_all(char * tittle,char * description)
{
	CURL * curl;
	CURLcode res;
	const char * HTTP_METHOD = "POST";
	const char * HTTP_URL = "http://api.tuisong.baidu.com/rest/3.0/push/all";
	const char * pAppKey = ANDROID_APP_KEY;
	const char * pSecretkey = ANDROID_SECRECT_KEY;
	char * msg = tittle;
	char msg_unicode[128] = { 0 };
	char description_unicode[256] = { 0 };
	char tmp_sign[64] = { 0 };
	char msg_buf[512] = { 0 };
	char msg_urlencode_buf[512] = { 0 };
	char pForm[1024] = { 0 };
	char pTemp[1024] = { 0 };
	char tmp_buf_src[1024] = { 0 };
	char tmp_buf_urlencode[1024] = { 0 };
	MD5_CTX ctx;
	char md5Value[128] = { 0 };
	if ((strlen(pAppKey) == 0) || (strlen(pSecretkey) == 0)){
		fprintf(stderr, "Enter correct baidu APIKEY!\n");
		return 1;
	}
	curl = curl_easy_init();
	if (curl){
		curl_easy_setopt(curl,CURLOPT_URL,HTTP_URL);
		struct curl_slist * http_headers = NULL;
		http_headers = curl_slist_append(http_headers, "Content-type: application/x-www-form-urlencoded;charset=utf-8");
		http_headers = curl_slist_append(http_headers, "User-Agent: BCCS_SDK/3.0 (Windows NT; build 7601 (Windows 7 Ultimate Edition Service Pack 1); i586) PHP/5.4.45 (Baidu Push SDK for PHP v3.0.0) cli/Unknown ZEND/2.4.0");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
		sprintf(pTemp,"apikey=%s",pAppKey);
		strcat(pForm, pTemp);
		time_t tick;
		tick = time(NULL);
		sprintf(pTemp,"&timestamp=%d",tick);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&device_type=3");
		strcat(pForm, pTemp);
		unsigned int ilen = 0;
		m_str2unicodestr(msg,strlen(msg),msg_unicode,&ilen);
		m_str2unicodestr(description,strlen(description),description_unicode,&ilen);
		sprintf(msg_buf,ANDROID_PUSHMESSAGEBODY,msg_unicode,description_unicode," ");
		UrlEncode(msg_buf,msg_urlencode_buf,sizeof(msg_buf),TRUE);
		sprintf(pTemp,"&msg=%s",msg_urlencode_buf);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&msg_type=1");
		strcat(pForm, pTemp);
		sprintf(tmp_buf_src,"%s%sapikey=%sdevice_type=3msg=%smsg_type=1timestamp=%d",HTTP_METHOD,HTTP_URL,pAppKey,msg_buf,tick);
		strncat(tmp_buf_src,pSecretkey,strlen(pSecretkey));
		UrlEncode(tmp_buf_src,tmp_buf_urlencode,sizeof(tmp_buf_urlencode),TRUE);
		MD5_Init(&ctx);
		MD5_Update(&ctx,tmp_buf_urlencode,strlen(tmp_buf_urlencode));
		MD5_Final((unsigned char *)md5Value, &ctx);
		bytes2Hex(tmp_sign, md5Value, 16);
		sprintf(pTemp,"&sign=%s",tmp_sign);
		strcat(pForm, pTemp);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS,pForm);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
		}
		curl_slist_free_all(http_headers);
		curl_easy_cleanup(curl);
	}
	return res;
}

int baidupush_ios_signle(char * channel_id,char * tittle,char * description)
{
	CURL * curl;
	CURLcode res;
	const char * HTTP_METHOD = "POST";
	const char * HTTP_URL = "http://api.tuisong.baidu.com/rest/3.0/push/single_device";
	const char * pAppKey = IOS_APP_KEY;
	const char * pSecretkey = IOS_SECRECT_KEY;
	char * msg = tittle;
	char msg_unicode[128] = { 0 };
	char description_unicode[256] = { 0 };
	char tmp_sign[64] = { 0 };
	char msg_buf[512] = { 0 };
	char msg_urlencode_buf[512] = { 0 };
	char pForm[1024] = { 0 };
	char pTemp[1024] = { 0 };
	char tmp_buf_src[1024] = { 0 };
	char tmp_buf_urlencode[1024] = { 0 };
	MD5_CTX ctx;
	char md5Value[128] = { 0 };
	if ((strlen(pAppKey) == 0) || (strlen(pSecretkey) == 0)){
		fprintf(stderr, "Enter correct baidu APIKEY!\n");
		return 1;
	}
	curl = curl_easy_init();
	if (curl){
		curl_easy_setopt(curl,CURLOPT_URL,HTTP_URL);
		struct curl_slist * http_headers = NULL;
		http_headers = curl_slist_append(http_headers, "Content-type: application/x-www-form-urlencoded;charset=utf-8");
		http_headers = curl_slist_append(http_headers, "User-Agent: BCCS_SDK/3.0 (Windows NT; build 7601 (Windows 7 Ultimate Edition Service Pack 1); i586) PHP/5.4.45 (Baidu Push SDK for PHP v3.0.0) cli/Unknown ZEND/2.4.0");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
		sprintf(pTemp,"apikey=%s",pAppKey);
		strcat(pForm, pTemp);
		time_t tick;
		tick = time(NULL);
		sprintf(pTemp,"&timestamp=%d",tick);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&channel_id=%s",channel_id);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&deploy_status=1");
		strcat(pForm, pTemp);
		sprintf(pTemp,"&device_type=4");
		strcat(pForm, pTemp);
		unsigned int ilen = 0;
		m_str2unicodestr(msg,strlen(msg),msg_unicode,&ilen);
		m_str2unicodestr(description,strlen(description),description_unicode,&ilen);
		sprintf(msg_buf,IOS_PUSHMESSAGEBODY,description_unicode," ");
		UrlEncode(msg_buf,msg_urlencode_buf,sizeof(msg_buf),TRUE);
		sprintf(pTemp,"&msg=%s",msg_urlencode_buf);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&msg_type=1");
		strcat(pForm, pTemp);
		sprintf(tmp_buf_src,"%s%sapikey=%schannel_id=%sdeploy_status=1device_type=4msg=%smsg_type=1timestamp=%d",HTTP_METHOD,HTTP_URL,pAppKey,channel_id,msg_buf,tick);
		strncat(tmp_buf_src,pSecretkey,strlen(pSecretkey));
		UrlEncode(tmp_buf_src,tmp_buf_urlencode,sizeof(tmp_buf_urlencode),TRUE);
		MD5_Init(&ctx);
		MD5_Update(&ctx,tmp_buf_urlencode,strlen(tmp_buf_urlencode));
		MD5_Final((unsigned char *)md5Value, &ctx);
		bytes2Hex(tmp_sign, md5Value, 16);
		sprintf(pTemp,"&sign=%s",tmp_sign);
		strcat(pForm, pTemp);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS,pForm);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
		}
		curl_slist_free_all(http_headers);
		curl_easy_cleanup(curl);
	}
	return res;
}


int baidupush_ios_all(char * tittle,char * description)
{
	CURL * curl;
	CURLcode res;
	const char * HTTP_METHOD = "POST";
	const char * HTTP_URL = "http://api.tuisong.baidu.com/rest/3.0/push/all";
	const char * pAppKey = IOS_APP_KEY;
	const char * pSecretkey = IOS_SECRECT_KEY;
	char * msg = tittle;
	char msg_unicode[128] = { 0 };
	char description_unicode[256] = { 0 };
	char tmp_sign[64] = { 0 };
	char msg_buf[512] = { 0 };
	char msg_urlencode_buf[512] = { 0 };
	char pForm[1024] = { 0 };
	char pTemp[1024] = { 0 };
	char tmp_buf_src[1024] = { 0 };
	char tmp_buf_urlencode[1024] = { 0 };
	char md5Value[128] = { 0 };
	MD5_CTX ctx;
	if ((strlen(pAppKey) == 0) || (strlen(pSecretkey) == 0)){
		fprintf(stderr, "Enter correct baidu APIKEY!\n");
		return 1;
	}
	curl = curl_easy_init();
	if (curl){
		curl_easy_setopt(curl,CURLOPT_URL,HTTP_URL);
		struct curl_slist * http_headers = NULL;
		http_headers = curl_slist_append(http_headers, "Content-type: application/x-www-form-urlencoded;charset=utf-8");
		http_headers = curl_slist_append(http_headers, "User-Agent: BCCS_SDK/3.0 (Windows NT; build 7601 (Windows 7 Ultimate Edition Service Pack 1); i586) PHP/5.4.45 (Baidu Push SDK for PHP v3.0.0) cli/Unknown ZEND/2.4.0");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
		sprintf(pTemp,"apikey=%s",pAppKey);
		strcat(pForm, pTemp);
		time_t tick;
		tick = time(NULL);
		sprintf(pTemp,"&timestamp=%d",tick);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&deploy_status=1");
		strcat(pForm, pTemp);
		sprintf(pTemp,"&device_type=4");
		strcat(pForm, pTemp);
		unsigned int ilen = 0;
		m_str2unicodestr(msg,strlen(msg),msg_unicode,&ilen);
		m_str2unicodestr(description,strlen(description),description_unicode,&ilen);
		sprintf(msg_buf,IOS_PUSHMESSAGEBODY,description_unicode," ");
		UrlEncode(msg_buf,msg_urlencode_buf,sizeof(msg_buf),TRUE);
		sprintf(pTemp,"&msg=%s",msg_urlencode_buf);
		strcat(pForm, pTemp);
		sprintf(pTemp,"&msg_type=1");
		strcat(pForm, pTemp);
		sprintf(tmp_buf_src,"%s%sapikey=%sdeploy_status=1device_type=4msg=%smsg_type=1timestamp=%d",HTTP_METHOD,HTTP_URL,pAppKey,msg_buf,tick);
		strncat(tmp_buf_src,pSecretkey,strlen(pSecretkey));
		UrlEncode(tmp_buf_src,tmp_buf_urlencode,sizeof(tmp_buf_urlencode),TRUE);		
		MD5_Init(&ctx);
		MD5_Update(&ctx,tmp_buf_urlencode,strlen(tmp_buf_urlencode));
		MD5_Final((unsigned char *)md5Value, &ctx);
		bytes2Hex(tmp_sign, md5Value, 16);
		sprintf(pTemp,"&sign=%s",tmp_sign);
		strcat(pForm, pTemp);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS,pForm);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
		}
		curl_slist_free_all(http_headers);
		curl_easy_cleanup(curl);
	}
	return res;
}

int baidupush_init(void)
{
	unsigned int ires = 0;
	curl_global_init(CURL_GLOBAL_ALL);
	return ires = 0;
}
