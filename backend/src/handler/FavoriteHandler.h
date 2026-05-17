#pragma once
#include "wfrest/HttpServer.h"

class FavoriteHandler {
public:
    static void register_routes(wfrest::HttpServer &server);
};
