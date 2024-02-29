/**
 * @file main.c
 * @brief Exmaple application to demonstrate HTTP Client API
 * @version 0.1
 * @date 2022-07-25
 * 
 * @copyright Copyright (c) 2022 Waybyte Solutions
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <lib.h>
#include <ril.h>
#include <os_api.h>
#include <network.h>
#include <proto/httpc.h>
#include <command.h>
#include <utils.h>

#ifdef SOC_RDA8910
#define STDIO_PORT "/dev/ttyUSB0"
#else
#define STDIO_PORT "/dev/ttyS0"
#endif

#define RESP_BUFLEN	1024
char response_bufer[RESP_BUFLEN];

/**
 * URC Handler
 * @param param1	URC Code
 * @param param2	URC Parameter
 */
static void urc_callback(unsigned int param1, unsigned int param2)
{
	switch (param1) {
	case URC_SYS_INIT_STATE_IND:
		if (param2 == SYS_STATE_SMSOK) {
			/* Ready for SMS */
		}
		break;
	case URC_SIM_CARD_STATE_IND:
		switch (param2) {
		case SIM_STAT_NOT_INSERTED:
			debug(DBG_OFF, "SYSTEM: SIM card not inserted!\n");
			break;
		case SIM_STAT_READY:
			debug(DBG_INFO, "SYSTEM: SIM card Ready!\n");
			break;
		case SIM_STAT_PIN_REQ:
			debug(DBG_OFF, "SYSTEM: SIM PIN required!\n");
			break;
		case SIM_STAT_PUK_REQ:
			debug(DBG_OFF, "SYSTEM: SIM PUK required!\n");
			break;
		case SIM_STAT_NOT_READY:
			debug(DBG_OFF, "SYSTEM: SIM card not recognized!\n");
			break;
		default:
			debug(DBG_OFF, "SYSTEM: SIM ERROR: %d\n", param2);
		}
		break;
	case URC_GSM_NW_STATE_IND:
		debug(DBG_OFF, "SYSTEM: GSM NW State: %d\n", param2);
		break;
	case URC_GPRS_NW_STATE_IND:
		break;
	case URC_CFUN_STATE_IND:
		break;
	case URC_COMING_CALL_IND:
		debug(DBG_OFF, "Incoming voice call from: %s\n", ((struct ril_callinfo_t *)param2)->number);
		/* Take action here, Answer/Hang-up */
		break;
	case URC_CALL_STATE_IND:
		switch (param2) {
		case CALL_STATE_BUSY:
			debug(DBG_OFF, "The number you dialed is busy now\n");
			break;
		case CALL_STATE_NO_ANSWER:
			debug(DBG_OFF, "The number you dialed has no answer\n");
			break;
		case CALL_STATE_NO_CARRIER:
			debug(DBG_OFF, "The number you dialed cannot reach\n");
			break;
		case CALL_STATE_NO_DIALTONE:
			debug(DBG_OFF, "No Dial tone\n");
			break;
		default:
			break;
		}
		break;
	case URC_NEW_SMS_IND:
		debug(DBG_OFF, "SMS: New SMS (%d)\n", param2);
		/* Handle New SMS */
		break;
	case URC_MODULE_VOLTAGE_IND:
		debug(DBG_INFO, "VBatt Voltage: %d\n", param2);
		break;
	case URC_ALARM_RING_IND:
		break;
	case URC_FILE_DOWNLOAD_STATUS:
		break;
	case URC_FOTA_STARTED:
		break;
	case URC_FOTA_FINISHED:
		break;
	case URC_FOTA_FAILED:
		break;
	case URC_STKPCI_RSP_IND:
		break;
	default:
		break;
	}
}

int http_test(const char *method, const char *url, const char *data, int type)
{
	int ret;
	struct httparg_t arg;

	printf("Sending HTTP %s request on %s\n\n", method, url);

	memset(&arg, 0, sizeof(arg));
	arg.url = url;
	arg.headers = "custom-header: some_value\r\n";
	arg.certs = NULL;
	arg.recv_headers = 0;
	arg.buflen = RESP_BUFLEN;
	arg.resp_buffer = response_bufer;
	if (data) {
		arg.mime = type;
		arg.submit_data = data;
		arg.submit_len = strlen(data);
	}
	memset(response_bufer, 0, RESP_BUFLEN);
	
	ret = httpc_submit(HTTP_CLIENT_DEFAULT, method, &arg);
	if (ret) {
		printf("HTTP %s request failed: %d\n", method, ret);
		return ret;
	}

	printf("HTTP %s Request ret: %d\n", method, ret);
	printf("HTTP response (%d bytes): %s\n\n", arg.buflen, response_bufer);

	return ret;
}

/**
 * Sample Task for HTTP Client API test
 * @param arg	Task Argument
 */
static void httpclient_task(void *arg)
{
	int timeout = 60;
	static const char json_data[] = "{\"string_key\":\"string value\",\"boolean_key\":true, \"int_key\":1234}";

	while (timeout && !network_isdataready()) {
		sleep(1);
		timeout--;
	}

	if (!network_isdataready()) {
		printf("Network ready timeout!");
		return;
	}
	
	while (1) {
		printf("Testing HTTP\n");
		http_test("GET", "http://httpbin.org/get", NULL, 0);
		sleep(1);
		http_test("POST", "http://httpbin.org/post", json_data, HTTP_MIME_TYPE_JSON);
		sleep(1);
		http_test("PATCH", "http://httpbin.org/patch", json_data, HTTP_MIME_TYPE_JSON);
		sleep(1);
		http_test("PUT", "http://httpbin.org/put", json_data, HTTP_MIME_TYPE_JSON);
		sleep(1);
		http_test("DELETE", "http://httpbin.org/delete", json_data, HTTP_MIME_TYPE_JSON);
		sleep(1);

		printf("Testing HTTPS\n");
		http_test("GET", "https://httpbin.org/get", NULL, 0);
		sleep(1);
		http_test("POST", "https://httpbin.org/post", json_data, HTTP_MIME_TYPE_JSON);
		sleep(1);
		http_test("PATCH", "https://httpbin.org/patch", json_data, HTTP_MIME_TYPE_JSON);
		sleep(1);
		http_test("PUT", "https://httpbin.org/put", json_data, HTTP_MIME_TYPE_JSON);
		sleep(1);
		http_test("DELETE", "https://httpbin.org/delete", json_data, HTTP_MIME_TYPE_JSON);
		sleep(10);
		break;
	}
}

static int do_upload(int argc, const char *argv[], struct cmdinfo_t *info)
{
	struct httpupload_t up;
	struct http_filemeta_t fmeta;

	fmeta.filename = strrchr(argv[1], '/') + 1;
	fmeta.filepath = argv[1];
	fmeta.mime_type = HTTP_MIME_TYPE_BINARY;
	fmeta.mime = NULL;
	fmeta.info = "Some information about the file";
	fmeta.timestamp = time(NULL);

	up.url = "http://httpbin.org/post";
	up.certs = NULL;
	up.buflen = RESP_BUFLEN;
	up.headers = NULL;
	up.meta = &fmeta;
	up.recv_headers = 0;
	up.resp_buffer = response_bufer;
	up.timeout = 0;
	
	int ret = httpc_upload(HTTP_CLIENT_DEFAULT, &up);
	printf("Upload ret: %d\n", ret);
	printf("Upload resp (%d): %s\n", up.buflen, up.resp_buffer);

	return 0;
}

CMD_ADD(upload, 1, 3, do_upload, "", "", CMD_TYPE_DEFAULT);

/**
 * Application main entry point
 */
int main(int argc, char *argv[])
{
	/*
	 * Initialize library and Setup STDIO
	 */
	logicrom_init(STDIO_PORT, urc_callback);
	/* enable GPRS */
	network_dataenable(TRUE);
	/* for download test */
	wget_init();
	
	printf("System Ready\n");

	/* Create Application tasks */
	os_task_create(httpclient_task, "httptest", NULL, FALSE);

	printf("System Initialization finished\n");

	while (1) {
		/* Main task */
		sleep(1);
	}
}

