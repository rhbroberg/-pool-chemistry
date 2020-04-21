#ifndef PushOTA_h
#define PushOTA_h

#include "Arduino.h"

class PushOTA
{
public:
    PushOTA();
    const bool enable();
    void disable();
    void handle();
    void setNetworking(const char *ssid, const char *password, const char *auth = NULL, const char *hostname = NULL);
    void setPort(const uint16_t port);

protected:
    const char *_ssid;
    const char *_password;
    const char *_auth;
    const char *_hostname;
};

#endif // PushOTA_h