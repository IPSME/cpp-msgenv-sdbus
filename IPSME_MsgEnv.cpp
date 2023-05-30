
#include <assert.h>
#include <systemd/sd-bus.h>
#include <stdarg.h>
#include <stdlib.h>

#include "IPSME_MsgEnv.h"
using IPSME_MsgEnv::RET_TYPE;
using IPSME_MsgEnv::tp_handler;

const char* kpsz_OBJ_PATH= "/dev/IPSME";
const char* kpsz_INTERFACE= "dev.IPSME";
const char* kpsz_SIGNAL= "IPSME";

sd_bus* p_bus_= nullptr;
sd_bus_slot* p_slot_= nullptr;

// The message m passed to the callback is only borrowed, that is, the callback should not call sd_bus_message_unref(3) on it. 
// If the callback wants to hold on to the message beyond the lifetime of the callback, it needs to call sd_bus_message_ref(3) to create a new reference.
int sd_handler_(sd_bus_message* p_msg, void* v_userdata, sd_bus_error* p_err)
{
	// printf("%s: \n", __func__); fflush(stdout);

	try {
		tp_handler p_handler= reinterpret_cast<tp_handler>(v_userdata);
		p_handler(p_msg);
	}
	catch (...) {
		assert(false);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

RET_TYPE IPSME_MsgEnv::subscribe(tp_handler p_handler)
{
	sd_bus_error err= SD_BUS_ERROR_NULL;
	sd_bus_message* p_m= nullptr;
	int i_r;

	i_r = sd_bus_open_system(&p_bus_);
	if (i_r < 0) {
		fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-i_r));
		goto finish;
	}


	i_r= sd_bus_match_signal(
			p_bus_, 
			&p_slot_,
		 	nullptr,			// const char *sender
		 	kpsz_OBJ_PATH,		// const char *path
		 	nullptr,			// const char *interface
		 	nullptr,			// const char *member
		 	sd_handler_, 
			reinterpret_cast<void*>(p_handler)
		);

	if (i_r < 0) {
		fprintf(stderr, "Failed to add match rule: %s\n", strerror(-i_r));
		goto finish;
	}

	printf("%s: %d= subscribe\n", __func__, i_r);

finish:
	sd_bus_error_free(&err);
	sd_bus_message_unref(p_m);
	sd_bus_unref(p_bus_);

	return i_r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

RET_TYPE IPSME_MsgEnv::unsubscribe(tp_handler p_handler);

sd_bus_message* IPSME_MsgEnv::sd_bus_message_new()
{
	sd_bus_message* p_m= nullptr;

	int i_r= sd_bus_message_new_signal(
			p_bus_,
			&p_m,
			kpsz_OBJ_PATH,	// const char *path,
			kpsz_INTERFACE,	// const char *interface
			kpsz_SIGNAL 	// const char *member
		);

	if (i_r < 0) {
		sd_bus_message_unrefp(&p_m);
		p_m= nullptr;
	}

	return p_m;
}

RET_TYPE IPSME_MsgEnv::sd_bus_send(sd_bus_message* p_m)
{
	if (! p_m)
		return EXIT_FAILURE;

	uint64_t ui64_cookie; // message identifier

	// bus only needs to be set when the message is sent to a different bus than the one it's attached to
	int i_r= sd_bus_send(nullptr, p_m, &ui64_cookie);

	return i_r;
}

// According to <systemd/sd-bus.h> on Ubuntu 20.04, the function sd_bus_emit_signalv() does NOT exist.
// sd_bus_message_appendv() DOES exist and isn't documented on https://www.freedesktop.org/software/systemd/man/sd-bus.html
// but HERE: https://www.freedesktop.org/software/systemd/man/sd_bus_message_append.html#

RET_TYPE IPSME_MsgEnv::publish(const char* psz_types, va_list va_l_argptr)
{
	_CLEANUP_(sd_bus_message_unrefp) sd_bus_message* p_m= nullptr;

	// printf("%s: %s \n", __func__, "va_list");

	int i_r;

    i_r= sd_bus_message_new_signal(
            p_bus_,
            &p_m,
            kpsz_OBJ_PATH,  // const char *path,
            kpsz_INTERFACE, // const char *interface
            kpsz_SIGNAL     // const char *member
        );

	if (i_r < 0)
		return i_r;

	i_r= sd_bus_message_appendv(p_m, psz_types, va_l_argptr);
    if (i_r < 0) {
        fprintf(stderr, "Failed to append va_list arguments: %s\n", strerror(-i_r));
        sd_bus_message_unref(p_m);
        return i_r;
    }

	uint64_t ui64_cookie; // message identifier

    // bus only needs to be set when the message is sent to a different bus than the one it's attached to
    i_r= sd_bus_send(nullptr, p_m, &ui64_cookie);

    return i_r;
}

RET_TYPE IPSME_MsgEnv::publish(const char* psz_types, ...)
{
	// printf("%s: %s \n", __func__, "...");

	va_list va_l_args;
	va_start(va_l_args, psz_types); // types last known argument

	int i_r= IPSME_MsgEnv::publish(psz_types, va_l_args);

	va_end(va_l_args);
	return i_r;
}

//	RET_TYPE IPSME_MsgEnv::publish(std::string str_msg)
//	{
//		_CLEANUP_(sd_bus_message_unrefp) sd_bus_message* p_m= IPSME_MsgEnv::sd_bus_message_new();
//	
//		if (! p_m)
//			return EXIT_FAILURE;
//	
//	    // printf("%s: [%s] \n", __func__, str_msg.c_str());
//	
//	    int i_r;
//	
//	    i_r = sd_bus_message_append_basic(p_m, SD_BUS_TYPE_STRING, str_msg.c_str());
//	    if (i_r < 0) {
//	        fprintf(stderr, "Failed to append string argument: %s\n", strerror(-i_r));
//	        sd_bus_message_unref(p_m);
//	        return i_r;
//	    }
//	
//		i_r= IPSME_MsgEnv::sd_bus_send(p_m);
//	
//		return i_r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
//	}

RET_TYPE IPSME_MsgEnv::process_requests()
{
	int i_r;

	do {
		i_r= sd_bus_process(p_bus_, nullptr);
		if (i_r < 0) {
			fprintf(stderr, "Failed to process bus: %s\n", strerror(-i_r));
			goto finish;
		}
	
		// if (i_r)
		//	printf("\n%s: %d= sd_bus_process \n", __func__, i_r);

	} while(i_r > 0); // we processed a request, try to process another one, right-away

	//TODO: For more complex programs either connect the bus connection object to an external event loop using sd_bus_get_fd(3) or to an sd-event(3) event loop using sd_bus_attach_event(3). https://www.freedesktop.org/software/systemd/man/sd_bus_wait.html#
    i_r= sd_bus_wait(p_bus_, (uint64_t) 100*1000); // 100msecs in micro
    if (i_r < 0) {
        fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-i_r));
        goto finish;
    }

	return i_r;
	//return i_r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;	

finish:
	sd_bus_slot_unref(p_slot_);
	sd_bus_unref(p_bus_);

	return i_r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;	
}
