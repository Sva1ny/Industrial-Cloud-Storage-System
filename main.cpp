#include <iostream>
#include <signal.h>
#include <unistd.h>
#include "OssManager.h"
#include "CloudiskServer.h"
#include "backend/src/util/Logger.h"

WFFacilities::WaitGroup g_waitGroup { 1 };
static bool g_running = true;

void sig_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM) {
        static int called = 0;
        if (called) {
            // 第二次 Ctrl+C 强制退出
            _exit(0);
        }
        called = 1;
        std::cerr << "\nShutting down server..." << std::endl;
        g_waitGroup.done();
    }
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);

    // 初始化日志
    Logger::instance().set_level(LOG_DEBUG);
    // Logger::instance().set_file("server.log");

    CloudiskServer svr;

    svr.register_modules();

    if (svr.track().start(8888) == 0) {
        svr.list_routes();
        g_waitGroup.wait();
        svr.stop();
        g_running = false;
    } else {
        std::cerr << "Error: server start failed!" << std::endl;
    }
}
