#pragma once
#include "wfrest/HttpServer.h"

class FileHandler {
public:
    static void register_routes(wfrest::HttpServer &server);
};
