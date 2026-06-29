
//
//  IPSME_MsgEnv.h  (sd-bus, class-based)
//
//  Created by dev on 2021-10-27.
//  Copyright © 2021 Root Interface. All rights reserved.
//

#ifndef IPSME_MSGENV_H
#define IPSME_MSGENV_H

#include <systemd/sd-bus.h>

// This MsgEnv has no global library to set up / tear down (unlike mosquitto), so it
// simply omits the static guard the MQTT MsgEnv uses -- callers stay MsgEnv-agnostic.
//
// Same class interface as cpp-msgenv-MQTT.git/IPSME_MsgEnv.h, implemented over sd-bus:
// the system bus carries dev.IPSME signals on object path /dev/IPSME.

class IPSME_MsgEnv {
public:
	typedef char const * const t_MSG;
	typedef void (*tp_callback)(t_MSG, void* p_void);
	typedef int RET_TYPE;

public:
	IPSME_MsgEnv();
	~IPSME_MsgEnv();

	bool subscribe(tp_callback p_callback, void* p_void);
	bool unsubscribe(tp_callback p_callback);

	bool publish(t_MSG);

	// Lower-level building blocks (parity with the prior namespace API): build a fresh dev.IPSME
	// signal message, append your own argument(s), then send it. sd_bus_send() consumes (unrefs) p_m.
	sd_bus_message* sd_bus_message_new();
	RET_TYPE        sd_bus_send(sd_bus_message* p_m);

	void process_msgs(int i_timeout= 0);

private:
	sd_bus*      _p_bus  = nullptr;
	sd_bus_slot* _p_slot = nullptr;

	tp_callback  _p_callback = nullptr;
	void*        _p_void     = nullptr;

	// sd-bus match trampoline: reads the string arg, calls the stored callback.
	static int _match_trampoline(sd_bus_message* p_m, void* userdata, sd_bus_error* p_err);
};

#endif // IPSME_MSGENV_H
