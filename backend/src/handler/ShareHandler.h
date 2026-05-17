#pragma once
#include "wfrest/HttpServer.h"

class ShareHandler {
public:
    static void register_routes(wfrest::HttpServer &server);
};
