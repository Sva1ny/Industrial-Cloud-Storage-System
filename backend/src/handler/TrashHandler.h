#pragma once
#include "wfrest/HttpServer.h"

class TrashHandler {
public:
    static void register_routes(wfrest::HttpServer &server);
};
