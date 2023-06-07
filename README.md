
# objc-msgenv-sdbus

This library contains the wrapper code for sending messages to the linux Dbus via `sd-bus` library. Under linux there is no standard pubsub installed, so there can be several different messaging environments (ME). 

> ### IPSME- Idempotent Publish/Subscribe Messaging Environment
> https://dl.acm.org/doi/abs/10.1145/3458307.3460966

#### Subscribing
```
bool  handler_str_(sd_bus_message* p_msg, std::string str_msg)
{
    printf("%s: [%s] \n", __func__, str_msg.c_str());

    return true;
}

std::string bus_message_str(sd_bus_message* p_msg) {
    const char* psz= nullptr;
    int i_r = sd_bus_message_read_basic(p_msg, SD_BUS_TYPE_STRING, &psz);
    if (i_r < 0)
        throw i_r;

    return psz;
}

void handler_(sd_bus_message* p_msg)
{
    // printf("%s: \n", __func__);

    try {
        char ch_type;
        if (sd_bus_message_peek_type(p_msg, &ch_type, nullptr) && (ch_type=='s') && handler_str_(p_msg, bus_message_str(p_msg) ))
            return;

    }
    catch (int &i_r) {
        printf("ERR: error is message execution: %s \n", strerror(-i_r));
    }
    catch (...) {
        printf("ERR: error is message execution \n");
        return;
    }

    printf("%s: DROP! \n", __func__);
}

// ...

IPSME_MsgEnv::subscribe(&handler_);  
```
#### Publishing
The syntax for native `sd-bus` message signatures can be found here: https://www.freedesktop.org/software/systemd/man/sd_bus_message_append.html#
The `IPSME_MsgEnv::publish` takes the same signature format. 

Sending a string would be this:
```
 IPSME_MsgEnv::publish("s", "...");
```

It is also possible to create a new `sd-bus` message and then use the `sd-bus` library functions to add data before publishing.

```
void publish(std::string str_msg)
{
    _CLEANUP_(sd_bus_message_unrefp) sd_bus_message* p_m= IPSME_MsgEnv::sd_bus_message_new();

    if (! p_m)
        return EXIT_FAILURE;

    // printf("%s: [%s] \n", __func__, str_msg.c_str());

    int i_r;

    i_r = sd_bus_message_append_basic(p_m, SD_BUS_TYPE_STRING, str_msg.c_str());
    if (i_r < 0) {
        fprintf(stderr, "Failed to append string argument: %s\n", strerror(-i_r));
        sd_bus_message_unref(p_m);
        return i_r;
    }

    i_r= IPSME_MsgEnv::sd_bus_send(p_m);

    return i_r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
```

#### Processing
For messages to be processed, the following call must be part of your run loop:
```
 while (! gb_quit_) {
    // ...
    IPSME_MsgEnv::process_requests();
    // ...
 }
```

