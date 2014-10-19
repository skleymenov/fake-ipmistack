/* Copyright (c) 2013, Zdenek Styblik
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Zdenek Styblik.
 * 4. Neither the name of the Zdenek Styblik nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZDENEK STYBLIK ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ZDENEK STYBLIK BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "fake-ipmistack/fake-ipmistack.h"

struct ipmi_channel {
	int number;
	uint8_t ptype;
	uint8_t mtype;
	uint8_t sessions;
	uint8_t capabilities;
	uint8_t priv_level;
	char desc[24];
} ipmi_channels[] __attribute__((unused)) = {
	{ 0xF0, 0x02, 0x01, 0x00, 0x00, 0x00, "IPMBv1.0, no-session" },
	{ 0xF1, 0x02, 0x04, 0x80, 0xC2, 0xF5, "802.3 LAN, m-session" },
	{ 0xF2, 0x02, 0x05, 0x40, 0x00, 0x00, "Serial/Modem, s-session" },
	{ 0xF3, 0x02, 0x02, 0x00, 0x00, 0x00, "ICMB no-session" },
	{ 0xF4, 0x04, 0x09, 0x00, 0x00, 0x00, "IPMI-SMBus no-session" },
	{ 0xF5, 0xFF },
	{ 0xF6, 0xFF },
	{ 0xF7, 0xFF },
	{ 0xF8, 0xFF },
	{ 0xF9, 0xFF },
	{ 0xFA, 0xFF },
	{ 0xFB, 0xFF },
	{ 0xFC, 0xFF },
	{ 0xFD, 0xFF },
	{ 0xFE, 0xFF },
	{ 0xFF, 0x05, 0x0C, 0x00, 0x00, 0x00, "KCS-SysIntf s-less" },
	{ -1 }
};

/* get_channel_by_number - return ipmi_channel structure based on given IPMI
 * Channel number.
 *
 * @chan_num: IPMI Channel number(needle)
 * @*ipmi_chan_ptr: pointer to ipmi_channel structure
 *
 * returns: 0 when channel is found, (-1) when it isn't/error
 */
int
get_channel_by_number(uint8_t chan_num, struct ipmi_channel *ipmi_chan_ptr)
{
	int i = 0;
	int rc = (-1);
	for (i = 0; ipmi_channels[i].number != (-1); i++) {
		if (ipmi_channels[i].number == chan_num
				&& ipmi_channels[i].ptype != 0xFF) {
			memcpy(ipmi_chan_ptr, &ipmi_channels[i],
					sizeof(struct ipmi_channel));
			rc = 0;
			break;
		}
	}
	return rc;
}

/* (22.23) Get Channel Access */
int
app_get_channel_access(struct dummy_rq *req, struct dummy_rs *rsp)
{
	struct ipmi_channel channel_t;
	uint8_t *data;
	uint8_t data_len = 2 * sizeof(uint8_t);
	if (req->msg.data_len != 2) {
		rsp->ccode = 0xC7;
		return (-1);
	}
	req->msg.data[1]|= 0x3F;
	if (req->msg.data[1] == 0x3F || req->msg.data[1] == 0xFF) {
		rsp->ccode = 0xCC;
		return (-1);
	}
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = 0xFF;
		printf("malloc fail\n");
		return (-1);
	}
	data[0] = req->msg.data[0];
	data[0]|= 0xF0;
	if (data[0] == 0xFE) {
		/* TODO - de-hard-code this */
		data[0] = 0xFF;
	}
	if (get_channel_by_number(data[0], &channel_t) != 0) {
		rsp->ccode = 0xCC;
		free(data);
		data = NULL;
		return (-1);
	}
	/* TODO - don't ignore req->data[1] -> return non-/volatile ACL */
	data[0] = channel_t.capabilities;
	data[1] = channel_t.priv_level;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (22.24) Get Channel Info */
int
app_get_channel_info(struct dummy_rq *req, struct dummy_rs *rsp)
{
	struct ipmi_channel channel_t;
	int8_t tmp = 0;
	uint8_t *data;
	uint8_t data_len = 9 * sizeof(uint8_t);
	if (req->msg.data_len != 1) {
		rsp->ccode = 0xC7;
		return (-1);
	}
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = 0xFF;
		printf("malloc fail\n");
		return (-1);
	}
	data[0] = req->msg.data[0];
	data[0]|= 0xF0;
	if (data[0] == 0xFE) {
		/* TODO - de-hard-code this */
		data[0] = 0xFF;
	}
	printf("[DEBUG] Channel is: %x\n", data[0]);
	if (get_channel_by_number(data[0], &channel_t) != 0) {
		printf("[ERROR] get channel by number\n");
		rsp->ccode = 0xCC;
		free(data);
		data = NULL;
		return (-1);
	}
	data[1] = channel_t.mtype;
	data[2] = channel_t.ptype;
	data[3] = channel_t.sessions;
	/* IANA */
	data[4] = 0xF2;
	data[5] = 0x1B;
	data[6] = 0x00;
	/* Auxilary Info */
	data[7] = 0;
	data[8] = 0;
	if (data[0] == 0xFF) {
		/* TODO - no idea, nothing */
		data[7] = 0xFF;
		data[8] = 0xFF;
	}
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (20.1) BMC Get Device ID */
int
mc_get_device_id(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int data_len = 14 * sizeof(uint8_t);
	uint8_t *data;
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = 0xFF;
		printf("malloc fail\n");
		return (-1);
	}
	memset(data, 0, data_len);
	data[0] = 12;
	data[1] = 0x80;
	data[2] = 0;
	data[5] = 0xff;
	/* TODO */
	/* data[0] - Device ID
	 * data[1] - Device Revision
	 * 	[7] - 1/0 - device provides SDRs
	 * 	[6:4] - reserved, return as 0
	 * 	[3:0] - Device Revision, binary encoded
	 * data[2] - FW Revision 1
	 * 	[7] - Device available
	 * 		0 = status ok
	 * 		1 - dev fw/self-init in progress
	 * 	[6:0] Major FW Revision, binary encoded
	 * data[3] - FW Revision 2, Minor, BCD encoded
	 * data[4] - IPMI version, BCD encoded, 51h =~ v1.5
	 * data[5] - Device Support
	 * 	[7] - Chassis support
	 * 	[6] - Bridge support
	 * 	[5] - IPMB Event Generator
	 * 	[4] - IPMB Event Receiver
	 * 	[3] - FRU Inventory Device
	 * 	[2] - SEL Device
	 * 	[1] - SDR Repository Device
	 * 	[0] - Sensor Device
	 * data[6-8] - Manufacturer ID, 20bit, binary encoded, LS first
	 * data[9-10] - Product ID, LS first
	 * data[11-14] - Auxilary FW Revision, MS first
	 */
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}
/* (20.8) Get Device GUID */
int
mc_get_device_guid(struct dummy_rq *req, struct dummy_rs *rsp)
{
	/* TODO - GUID generator ???
	 * http://download.intel.com/design/archives/wfm/downloads/base20.pdf
	 */
	int data_len = 15;
	uint8_t *data = NULL;
	data = malloc(data_len);
	if (data == NULL) {
		printf("malloc fail\n");
		rsp->ccode = 0xFF;
		return (-1);
	}
	memset(&data, 0, data_len);
	rsp->data_len = data_len;
	rsp->data = data;
	return 0;
}

/* (20.2) BMC Cold and (20.3) Warm Reset */
int
mc_reset(struct dummy_rq *req, struct dummy_rs *rsp)
{
	rsp->data_len = 0;
	if (req->msg.cmd == BMC_RESET_COLD) {
		/* do nothing */
	} else if (req->msg.cmd == BMC_RESET_WARM) {
		/* do nothing */
	} else {
		printf("[ERROR] Invalid command '%u'.\n",
				req->msg.cmd);
		rsp->ccode = 0xC1;
		return (-1);
	}
	return 0;
}

/* (20.4) BMC Selftest */
int
mc_selftest(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 2 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		printf("malloc fail\n");
		rsp->ccode = 0xFF;
		return (-1);
	}
	data[0] = 0x57;
	data[1] = 0x04;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

int
netfn_app_main(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int rc = 0;
	rsp->msg.netfn = req->msg.netfn + 1;
	rsp->msg.cmd = req->msg.cmd;
	rsp->msg.lun = req->msg.lun;
	rsp->ccode = 0;
	rsp->data_len = 0;
	rsp->data = NULL;
	switch (req->msg.cmd) {
	case APP_GET_CHANNEL_ACCESS:
		rc = app_get_channel_access(req, rsp);
		break;
	case APP_GET_CHANNEL_INFO:
		rc = app_get_channel_info(req, rsp);
		break;
	case BMC_GET_DEVICE_ID:
		rc = mc_get_device_id(req, rsp);
		break;
	case BMC_RESET_COLD:
	case BMC_RESET_WARM:
		rc = mc_reset(req, rsp);
		break;
	case BMC_SELFTEST:
		rc = mc_selftest(req, rsp);
		break;
	case BMC_GET_DEVICE_GUID:
		rc = mc_get_device_guid(req, rsp);
		break;
	default:
		rsp->ccode = 0xC1;
		rc = (-1);
	}
	return rc;
}
