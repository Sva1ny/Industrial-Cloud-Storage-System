#pragma once
#include "wfrest/HttpServer.h"

class SearchHandler {
public:
    static void register_routes(wfrest::HttpServer &server);
};
