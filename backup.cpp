#include <iostream>
#include <string>
#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <nlohmann/json.hpp>
#include "OssManager.h"

using namespace std;
using namespace AmqpClient;

int main()
{
    string uri = "amqp://guest:guest@localhost:5672/%2f";
    const string& q = "oss.queue";

    Channel::ptr_t channel = Channel::CreateFromUri(uri);

    // 绑定队列，channel会消耗队列q中的消息
    channel->BasicConsume(q);

    OssManager* oss = OssManager::get_instance();
    for(;;) {
        // 获取一封消息
        // 如果队列中没有消息，则一直等待
        Envelope::ptr_t envelope = channel->BasicConsumeMessage();

        if (envelope && envelope->Message()) {
            nlohmann::json message = nlohmann::json::parse(envelope->Message()->Body());
            string object = message["object"];
            string file = message["file"];
#ifdef DEBUG
            cout << "object: " << object << ", file: " << file << endl;
#endif
            if (!oss->upload_object("peanutixx-oss-demo", object, file)) {
                cerr << "Error: upload failed! object: " << object 
                     << ", file: " << file << endl;
            }
        }
    }

    OssManager::destroy_instance();
}