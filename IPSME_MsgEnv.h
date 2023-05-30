
//
//  IPSME_MsgEnv.h
//
//  Created by dev on 2021-10-27.
//  Copyright © 2021 Root Interface. All rights reserved.
//

#include <systemd/sd-bus.h>

#ifndef IPSME_MSGENV_H
#define IPSME_MSGENV_H

#define _CLEANUP_(f) __attribute__((cleanup(f)))
//usage: _CLEANUP_(sd_bus_message_unrefp) sd_bus_message* p_m= nullptr;

namespace IPSME_MsgEnv
{
	typedef int RET_TYPE;
	typedef void (*tp_handler)(sd_bus_message* p_m);

	RET_TYPE subscribe(IPSME_MsgEnv::tp_handler p_handler);
	RET_TYPE unsubscribe(IPSME_MsgEnv::tp_handler p_handler);

	sd_bus_message* sd_bus_message_new();
	RET_TYPE sd_bus_send(sd_bus_message* p_m);

	// arg docs: https://www.freedesktop.org/software/systemd/man/sd_bus_message_append.html#
	RET_TYPE publish(const char *types, va_list va_l_argptr);
	RET_TYPE publish(const char *types, ...);

	RET_TYPE process_requests();
}

#endif // IPSME_MSGENV_H
