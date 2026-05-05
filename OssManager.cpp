#include "OssManager.h"

using namespace std;

// 初始化静态成员变量
OssManager* OssManager::instance = nullptr;
mutex OssManager::m_mutex;

OssManager* OssManager::get_instance()
{   
    // 第一次检查：如果实例已经存在，直接返回
    if (instance == nullptr)
    {
        lock_guard<mutex> lock(m_mutex);
        // 第二次检查：防止在多线程环境下重复创建实例
        if (instance == nullptr)
        {
            instance = new OssManager();
        }
    }
    return instance;
}

void OssManager::destroy_instance() 
{
    lock_guard<mutex> lock(m_mutex);
    if (instance != nullptr) 
    {
        delete instance;
        instance = nullptr;
        AlibabaCloud::OSS::ShutdownSdk(); // 释放SDK资源
    }
}

OssManager::OssManager()
{
    // 1. 初始化SDK
    AlibabaCloud::OSS::InitializeSdk(); 

    string endpoint = "oss-cn-wuhan-lr.aliyuncs.com";
    string region = "cn-wuhan";
    string accessKeyId = "LTAI5t7SqTtbX4UcwBGnWRuG";
    string accessKeySecret = "u32JnGRv5zgWf5y0nDZcOXlrlzzepv";
    AlibabaCloud::OSS::ClientConfiguration conf;
    m_client = std::make_unique<AlibabaCloud::OSS::OssClient>(endpoint, accessKeyId, accessKeySecret, conf);
            
    m_client->SetRegion(region);
}

bool OssManager::upload_object(const string& bucket, const string& object, const string& file) // 上传文件
{
    auto outcome = m_client->PutObject(bucket, object, file);
    return outcome.isSuccess(); 
}

bool OssManager::upload_object(const string& bucket, const string& object, std::shared_ptr<std::iostream> content)
{
    auto outcome = m_client->PutObject(bucket, object, content);
    return outcome.isSuccess();
}