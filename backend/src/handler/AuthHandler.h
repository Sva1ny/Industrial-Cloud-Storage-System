#pragma once
#include "wfrest/HttpServer.h"

class AuthHandler {
public:
    static void register_routes(wfrest::HttpServer &server);
};
