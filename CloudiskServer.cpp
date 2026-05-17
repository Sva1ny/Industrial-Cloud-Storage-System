#include <workflow/MySQLResult.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/MySQLUtil.h>
#include <wfrest/PathUtil.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <sys/stat.h>
#include <sys/types.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <cctype>
#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include "CloudiskServer.h"
#include "CryptoUtil.h"
#include "OssManager.h"
#include "UserService.srpc.h"
#include "UserService.pb.h"
#include "Util.h"
#include "Config.h"
#include "backend/src/util/Logger.h"
#include "backend/src/search/InvertedIndex.h"
#include "backend/src/handler/AuthHandler.h"
#include "backend/src/handler/FileHandler.h"
#include "backend/src/handler/TrashHandler.h"
#include "backend/src/handler/ShareHandler.h"
#include "backend/src/handler/SearchHandler.h"
#include "backend/src/handler/FavoriteHandler.h"

using namespace std;
using namespace wfrest;
using namespace protocol;
using namespace std::placeholders;
using namespace AmqpClient;

static const std::string RABBITMQ_URL = rabbitmq_url();
static const string MYSQL_URL = mysql_url();
static const int RETRY_MAX = 3;

// URL 解码：将 %XX 转换为对应的字符
static string url_encode(const string& src)
{
    string result;
    for (char c : src) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            result += c;
        else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            result += buf;
        }
    }
    return result;
}

static string url_decode(const string& src)
{
    string result;
    result.reserve(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '%' && i + 2 < src.size()) {
            unsigned int c;
            sscanf(src.c_str() + i + 1, "%2x", &c);
            result += static_cast<char>(c);
            i += 2;
        } else {
            result += src[i];
        }
    }
    return result;
}

// 通过 Consul 发现 UserService 地址
// 返回 (ip, port)，失败时返回 ("", 0)
static pair<string, unsigned short> discover_user_service()
{
    string ip;
    unsigned short port = 0;
    WFFacilities::WaitGroup waitGroup { 1 };
    string consulURL = "http://127.0.0.1:8500/v1/health/service/UserService?passing=true";
    WFHttpTask* consulTask = WFTaskFactory::create_http_task(consulURL, 3, 3, [&](WFHttpTask* task)
    {
        if (task->get_state() != WFT_STATE_SUCCESS) {
            waitGroup.done();
            return ;
        }
        HttpResponse* response = task->get_resp();
        string body = HttpUtil::decode_chunked_body(response);
        nlohmann::json data = nlohmann::json::parse(body);
        if (data.size() == 0) {
            waitGroup.done();
            return ;
        }
        ip = data[0]["Service"]["Address"];
        port = data[0]["Service"]["Port"];
        waitGroup.done();
    });
    consulTask->start();
    waitGroup.wait();
    return { ip, port };
}

void CloudiskServer::register_modules()
{
    // 设置静态资源的路由
    register_static_resources_module();
    register_signup_module();
    register_signin_module();
    register_userinfo_module();
    register_fileupload_module();
    register_filelist_module();
    register_filedownload_module();
    register_filedelete_module();

    // Phase 0+: 新的模块化 API 端点
    AuthHandler::register_routes(m_server);
    FileHandler::register_routes(m_server);
    TrashHandler::register_routes(m_server);
    ShareHandler::register_routes(m_server);
    SearchHandler::register_routes(m_server);
    FavoriteHandler::register_routes(m_server);
}

void CloudiskServer::register_static_resources_module()
{

    m_server.GET("/user/signup", [](const HttpReq *, HttpResp * resp){
        resp->File("static/view/signup.html");
    });

    m_server.GET("/static/view/signin.html", [](const HttpReq *, HttpResp * resp){
        resp->File("static/view/signin.html");
    });

    m_server.GET("/static/view/home.html", [](const HttpReq *, HttpResp * resp){
        resp->File("static/view/home.html");
    });

    m_server.GET("/static/js/auth.js", [](const HttpReq *, HttpResp * resp){
        resp->File("static/js/auth.js");
    });

    // Vue 3 SPA
    m_server.GET("/web", [](const HttpReq *, HttpResp *resp){
        resp->File("web/index.html");
    });
    m_server.GET("/web/", [](const HttpReq *, HttpResp *resp){
        resp->File("web/index.html");
    });
    m_server.GET("/web/css/app.css", [](const HttpReq *, HttpResp *resp){
        resp->File("web/css/app.css");
    });
    m_server.GET("/web/js/api.js", [](const HttpReq *, HttpResp *resp){
        resp->File("web/js/api.js");
    });
    m_server.GET("/web/js/store.js", [](const HttpReq *, HttpResp *resp){
        resp->File("web/js/store.js");
    });
    m_server.GET("/web/js/utils.js", [](const HttpReq *, HttpResp *resp){
        resp->File("web/js/utils.js");
    });
    m_server.GET("/web/js/app.js", [](const HttpReq *, HttpResp *resp){
        resp->File("web/js/app.js");
    });
    m_server.GET("/web/js/components/FileToolbar.js", [](const HttpReq *, HttpResp *resp){
        resp->File("web/js/components/FileToolbar.js");
    });
    m_server.GET("/web/js/components/FileTable.js", [](const HttpReq *, HttpResp *resp){
        resp->File("web/js/components/FileTable.js");
    });
    m_server.GET("/web/js/views/AuthView.js", [](const HttpReq *, HttpResp *resp){
        resp->File("web/js/views/AuthView.js");
    });
    m_server.GET("/web/js/views/DashboardView.js", [](const HttpReq *, HttpResp *resp){
        resp->File("web/js/views/DashboardView.js");
    });

    m_server.GET("/static/img/avatar.jpeg", [](const HttpReq *, HttpResp * resp){
        resp->File("static/img/avatar.jpeg");
    });

    m_server.GET("/file/upload", [](const HttpReq *, HttpResp * resp){
        resp->File("static/view/index.html");
    });

    m_server.Static("/file/upload_files","static/view/upload_files");
}

/*********************************************************************************
 *                               注册                                            *
 *********************************************************************************/

void CloudiskServer::register_signup_module()
{
    m_server.POST("/user/signup", [](const HttpReq* req, HttpResp* resp)
    {
        if (req->content_type() != APPLICATION_URLENCODED) {
            resp->set_status(HttpStatusBadRequest);  
            resp->String("<html>400 Bad Request</html>");
            return;
        }
        // 1. 解析表单数据(application/x-www-form-urlencoded)，获取用户名和密码
        map<string, string>& data = req->form_kv();
        string& username = data["username"];
        string& password = data["password"];
#ifdef DEBUG
        cout << "[INFO] username: " << username << ", password: " << password << endl; /* 调试信息 */
#endif
        // 2. 校验用户名和密码(用户名是否在黑名单内，密码是否符合强度要求...)
        // 这些校验可能前端也会做 (提升用户体验)
        // 但是后端永远不要相信前端传过来的数据 (因为很容易绕过前端，直接给后端发发送请求，比如用 curl)
        if (username == "" || password == "") {
            resp->set_status(HttpStatusBadRequest);
            resp->String("<html>400 Bad Request</html>");
            return ;
        }

        // 3. 访问Consul获取服务的Address和Port
        auto [ip, port] = discover_user_service();
        if (ip == "" || port == 0) {
            resp->set_status(HttpStatusInternalServerError);
            resp->String("<html>500 Internal Server Error</html>");
            return ;
        }

        // 4.校验通过后，远程同步调用UserService的sign_up()方法
        UserService::SRPCClient client{ ip.c_str(), port };
        // 设置请求
        UserRequest request;
        request.set_username(username);
        request.set_password(password);
        // 同步调用
        UserResponse response;
        srpc::RPCSyncContext ctx;
        client.sign_up(&request, &response, &ctx);

        // 4. 返回响应
        if (ctx.success && response.success()) {
            resp->String("SUCCESS");
        } else {
            resp->set_status(HttpStatusBadRequest);
            resp->String("<html>用户名已存在</html>");
        }
    });
}


/*********************************************************************************
 *                               登录                                            *
 *********************************************************************************/
void CloudiskServer::register_signin_module()
{
    // 精确路由
    m_server.POST("/user/signin", [](const HttpReq* req, HttpResp* resp, SeriesWork* series)
    {
        if (req->content_type() != APPLICATION_URLENCODED) {
            resp->set_status(HttpStatusBadRequest);  /* 400 Bad Request */
            resp->String("<html>400 Bad Request</html>");
            return ;
        }

        // 1. 解析表单数据(application/x-www-form-urlencoded)，获取用户名和密码
        map<string, string>& data = req->form_kv();
        string& username = data["username"];
        string& password = data["password"];
#ifdef DEBUG
        cout << "[INFO] username: " << username << ", password: " << password << endl; /* 调试信息 */
#endif

        // 2. 校验用户名和密码(用户名是否在黑名单内，密码是否符合强度要求...)
        // 这些校验可能前端也会做 (提升用户体验)
        // 但是后端永远不要相信前端传过来的数据 (因为很容易绕过前端，直接给后端发发送请求，比如用 curl)
        if (username == "" || password == "") {
            resp->set_status(HttpStatusBadRequest);  /* 400 Bad Request */
            resp->String("<html>400 Bad Request</html>");
            return ;
        }

        // 3. 访问Consul获取服务的Address和Port
        auto [ip, port] = discover_user_service();
        if (ip == "" || port == 0) {
            resp->set_status(HttpStatusInternalServerError);
            resp->String("<html>500 Internal Server Error</html>");
            return ;
        }

        // 4.校验通过后，远程同步调用UserService的sign_in()方法
        UserService::SRPCClient client{ ip.c_str(), port };
        // 设置请求
        UserRequest request;
        request.set_username(username);
        request.set_password(password);
        // 同步调用
        UserResponse response;
        srpc::RPCSyncContext ctx;
        client.sign_in(&request, &response, &ctx);

        // 4. 返回响应
        if (ctx.success && response.success()) {
            nlohmann::json data;
            data["Username"] = response.username();
            data["Token"] = response.token();
            data["Location"] = "/static/view/home.html";    /* 跳转到用户中心页面 */

            nlohmann::json json;
            json["data"] = data;
            resp->headers["Content-Type"] = "application/json; charset=utf-8";
            resp->String(json.dump());
        } else {
            resp->set_status(HttpStatusBadRequest);
            resp->String("<html>用户名或密码错误</html>");
        }
    });
}


/*********************************************************************************
 *                               用户信息                                        *
 *********************************************************************************/
void CloudiskServer::register_userinfo_module()
{
    m_server.GET("/user/info", [](const HttpReq* req, HttpResp* resp)
    {
        string username = req->query("username");   
        string token = req->query("token");
#ifdef DEBUG
        cout << "username: " << username 
            << "token: " << token << endl;
#endif
        // 校验Token
        User user;
        if (!CryptoUtil::verify_token(token, user)) {
            resp->set_status(HttpStatusBadRequest);
            resp->String("<html>400 Bad Request</html>");
            return ;
        }
        
        nlohmann::json data;
        data["Username"] = user.username;
        data["SignupAt"] = user.createdAt;

        nlohmann::json json;
        json["data"] = data;
        resp->headers["Content-Type"] = "application/json; charset=utf-8";
        resp->String(json.dump(2));
    });
}

/*********************************************************************************
 *                               上传文件                                        *
 *********************************************************************************/

void CloudiskServer::register_fileupload_module()
{
    m_server.POST("/file/upload", [](const HttpReq* req, HttpResp* resp)
    {
        // 1. 解析请求参数
        string username = req->query("username");
        string token = req->query("token");
        // 2. 校验token
        User user{};
        if (!CryptoUtil::verify_token(token, user)) {
            resp->set_status(HttpStatusUnauthorized);
            resp->String("<html>401 Unauthorized</html>");
            return ;
        }
        // 3. 处理文件
        if (req->content_type() != MULTIPART_FORM_DATA) {
            resp->set_status(HttpStatusBadRequest);
            resp->String("<html>400 Bad Request</html>");
            return ;
        }
        // using Form = std::map<std::string, std::pair<std::string, std::string>>
        Form& form = req->form();
        for (const auto& [_, file] : form) {
            const auto& [filename, content] = file;

            // 文件大小校验（最大 100MB）
            const size_t MAX_FILE_SIZE = 100 * 1024 * 1024;
            if (content.size() > MAX_FILE_SIZE) {
                resp->set_status(HttpStatusBadRequest);
                resp->String("<html>400 File too large (max 100MB)</html>");
                return ;
            }

            string hashcode = CryptoUtil::generate_hashcode(content.c_str(), content.size());
            
            string directory = "files/" + user.username + "/";  /* 为每个用户单独创建一个文件夹 */

            // access测试文件是否可以访问, F_OK用来判断文件是否存在
            if (access(directory.c_str(), F_OK) == -1) {
                // 如果文件夹不存在，则创建文件夹
                mkdir(directory.c_str(), 0777);
            }

            string filepath = directory + PathUtil::base(filename);
#ifdef DEBUG
            cout << "filepath: " << filepath << endl;
#endif
            int fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) {
                resp->set_status(500);
                resp->String("<html>500 Internal Server Error</html>");
                return ;
            }
            // 写入文件
            write(fd, content.c_str(), content.size());
            close(fd);

            // [OSS备份]: 异步备份，往消息队列中写入一条消息
            try {
                Channel::ptr_t channel = Channel::CreateFromUri(RABBITMQ_URL);
                nlohmann::json obj;
                obj["object"] = filepath;
                obj["file"] = filepath;
                BasicMessage::ptr_t message = BasicMessage::Create(obj.dump());
                string exchange = "oss.direct";
                string routingKey = "oss";
                channel->BasicPublish(exchange, routingKey, message);
            } catch (std::exception& e) {
                cerr << "[WARN] RabbitMQ publish failed: " << e.what() << endl;
            }

            // 写`tbl_file`表
            string sql = "REPLACE INTO tbl_file (uid, filename, hashcode, size) VALUES ("
                + std::to_string(user.id) + ", '"
                + mysql_escape(filename) + "', '"
                + hashcode + "', "
                + std::to_string(content.size()) + ")";

            cout << "[SQL] " << sql << endl;

            // Archive old version + upsert into tbl_file_v2
            std::string file_basename = PathUtil::base(filename);
            std::string storage_path = "files/" + user.username + "/" + file_basename;
            std::string archive_sql = "INSERT INTO tbl_file_history (file_id, version, hashcode, size, storage_path) "
                                      "SELECT id, 0, hashcode, size, storage_path FROM tbl_file_v2 "
                                      "WHERE uid = " + std::to_string(user.id) +
                                      " AND filename = '" + mysql_escape(file_basename) +
                                      "' AND parent_id = 0 AND tomb = 0 AND is_folder = 0";

            resp->MySQL(MYSQL_URL, archive_sql, [resp, sql, uid = user.id, file_basename, hashcode, content_size = content.size(), storage_path](MySQLResultCursor*)
            {
                // REPLACE INTO tbl_file (legacy)
                resp->MySQL(MYSQL_URL, sql, [resp, uid, file_basename, hashcode, content_size, storage_path](MySQLResultCursor* cursor2)
                {
                    // Upsert into tbl_file_v2
                    std::string v2_sql = "INSERT INTO tbl_file_v2 (uid, parent_id, filename, hashcode, size, storage_path) "
                                        "VALUES (" + std::to_string(uid) + ", 0, '"
                                        + mysql_escape(file_basename) + "', '"
                                        + hashcode + "', "
                                        + std::to_string(content_size) + ", '"
                                        + storage_path + "') "
                                        "ON DUPLICATE KEY UPDATE hashcode = VALUES(hashcode), "
                                        "size = VALUES(size), storage_path = VALUES(storage_path)";

                    resp->MySQL(MYSQL_URL, v2_sql, [resp, uid, file_basename, storage_path](MySQLResultCursor*)
                    {
                        // Get file_id for indexing
                        std::string id_sql = "SELECT id FROM tbl_file_v2 WHERE uid = "
                            + std::to_string(uid) + " AND filename = '"
                            + mysql_escape(file_basename) + "' AND parent_id = 0 AND tomb = 0";

                        resp->MySQL(MYSQL_URL, id_sql, [resp, storage_path](MySQLResultCursor *cursor)
                        {
                            int64_t file_id = 0;
                            if (cursor->get_cursor_status() == MYSQL_STATUS_GET_RESULT) {
                                std::vector<MySQLCell> row;
                                if (cursor->fetch_row(row))
                                    file_id = row[0].as_ulonglong();
                            }

                            if (file_id > 0) {
                                // Index text content
                                auto index_data = InvertedIndex::prepare_index(storage_path, file_id);
                                if (!index_data.insert_text_sql.empty()) {
                                    resp->MySQL(MYSQL_URL, index_data.delete_index_sql, [resp, index_data](MySQLResultCursor*)
                                    {
                                        resp->MySQL(MYSQL_URL, index_data.insert_text_sql, [resp, index_data](MySQLResultCursor*)
                                        {
                                            // Insert index entries (batch into one query for simplicity)
                                            if (!index_data.insert_index_sqls.empty()) {
                                                // Build multi-row INSERT
                                                std::string batch_sql = "INSERT INTO tbl_inverted_index (term, file_id, freq, positions) VALUES ";
                                                for (size_t i = 0; i < index_data.insert_index_sqls.size(); i++) {
                                                    // Each sql is "INSERT INTO tbl_inverted_index ... VALUES ('term', 26, 1, '0')"
                                                    // Extract the VALUES part
                                                    auto val_pos = index_data.insert_index_sqls[i].find("VALUES");
                                                    if (val_pos != std::string::npos) {
                                                        if (i > 0) batch_sql += ",";
                                                        batch_sql += index_data.insert_index_sqls[i].substr(val_pos + 6);
                                                    }
                                                }

                                                resp->MySQL(MYSQL_URL, batch_sql, [resp](MySQLResultCursor*)
                                                {
                                                    nlohmann::json json;
                                                    json["success"] = true;
                                                    json["redirect"] = "/static/view/home.html";
                                                    resp->headers["Content-Type"] = "application/json; charset=utf-8";
                                                    resp->String(json.dump());
                                                });
                                            } else {
                                                nlohmann::json json;
                                                json["success"] = true;
                                                json["redirect"] = "/static/view/home.html";
                                                resp->headers["Content-Type"] = "application/json; charset=utf-8";
                                                resp->String(json.dump());
                                            }
                                        });
                                    });
                                    return;
                                }
                            }

                            nlohmann::json json;
                            json["success"] = true;
                            json["redirect"] = "/static/view/home.html";
                            resp->headers["Content-Type"] = "application/json; charset=utf-8";
                            resp->String(json.dump());
                        });
                    });
                });
            });
        }
    });
}


/*********************************************************************************
 *                               文件列表                                        *
 *********************************************************************************/
void filelist_callback(HttpResp* resp, MySQLResultCursor* cursor)
{
    /* MySQL任务失败 */
    if (cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
        resp->set_status(HttpStatusInternalServerError);
        resp->String("<html>500 Internal Server Error");
        return ;
    } 

    /* 任务执行成功 */
    vector<MySQLCell> record;

    nlohmann::json result = nlohmann::json::array();
    while (cursor->fetch_row(record)) {
        nlohmann::json file;
        file["FileName"] = record[0].as_string();
        file["FileHash"] = record[1].as_string();
        file["FileSize"] = record[2].as_ulonglong();
        file["UploadAt"] = record[3].as_datetime();
        file["LastUpdated"] = record[4].as_datetime();
        result.push_back(std::move(file));
    }
    resp->headers["Content-Type"] = "application/json; charset=utf-8";
    resp->String(result.dump(2));
}

void CloudiskServer::register_filelist_module()
{
    m_server.POST("/file/query", [](const HttpReq* req, HttpResp* resp)
    {
        // 1. 解析请求
        string username = req->query("username");
        string token = req->query("token");
        string limit = req->form_kv()["limit"];
        string offset = req->form_kv()["offset"];

        // 2. 校验Token
        User user;
        if (!CryptoUtil::verify_token(token, user)) {
            resp->set_status(HttpStatusUnauthorized);
            resp->String("<html>401 Unauthorized</html>");
            return ;
        }
        // 3. 校验 limit 和 offset 为数字
        auto parse_num = [](const string& s, int default_val) -> int {
            if (s.empty()) return default_val;
            int n = 0;
            for (char c : s) {
                if (!isdigit(c)) return -1;
                n = n * 10 + (c - '0');
            }
            return n;
        };
        int limit_num = parse_num(limit, 15);
        int offset_num = parse_num(offset, 0);
        if (limit_num < 0 || offset_num < 0) {
            resp->set_status(HttpStatusBadRequest);
            resp->String("<html>400 Bad Request</html>");
            return;
        }
        // 限制最大查询量
        if (limit_num > 100) limit_num = 100;

        string sql = "SELECT filename, hashcode, size, created_at, last_update FROM tbl_file WHERE uid="
            + std::to_string(user.id)
            + " LIMIT " + std::to_string(limit_num)
            + " OFFSET " + std::to_string(offset_num);

        cout << "[SQL] " << sql << endl;

        resp->MySQL(
            MYSQL_URL,
            sql,
            std::bind(filelist_callback, resp, _1)
        );
    });
}

/*********************************************************************************
 *                               下载文件                                        *
 *********************************************************************************/
void CloudiskServer::register_filedownload_module()
{
    m_server.GET("/file/download", [](const HttpReq* req, HttpResp* resp)
    {
        // 1. 获取请求参数
        string filename = url_decode(req->query("filename"));
        const string& token = req->query("token");

        // 2. 验证 token，用 token 中的 username 而非 URL 中的
        User user;
        if (!CryptoUtil::verify_token(token, user)) {
            resp->set_status(HttpStatusUnauthorized);
            resp->headers["Content-Type"] = "application/json; charset=utf-8";
            resp->String(R"({"success":false,"error":"unauthorized"})");
            return;
        }

        // 3. 文件路径使用 token 中的用户名（安全）
        string filepath = "files/" + user.username + "/" + PathUtil::base(filename);

        // 4. 验证文件存在
        if (access(filepath.c_str(), F_OK) != 0) {
            resp->set_status(HttpStatusNotFound);
            resp->headers["Content-Type"] = "application/json; charset=utf-8";
            resp->String(R"({"success":false,"error":"file not found"})");
            return;
        }

        // 5. 兼容非 ASCII 文件名的 Content-Disposition（RFC 5987）
        string disposition = "attachment; filename=\""
            + url_encode(PathUtil::base(filename)) + "\""
            "; filename*=UTF-8''" + url_encode(PathUtil::base(filename));
        resp->set_header_pair("Content-Disposition", disposition);
        resp->File(filepath);
    });
}

/*********************************************************************************
 *                               删除文件                                        *
 *********************************************************************************/
void CloudiskServer::register_filedelete_module()
{
    m_server.POST("/file/delete", [](const HttpReq* req, HttpResp* resp)
    {
        string username = req->query("username");
        string token = req->query("token");
        string filename = url_decode(req->form_kv()["filename"]);

        User user;
        if (!CryptoUtil::verify_token(token, user)) {
            resp->set_status(HttpStatusUnauthorized);
            resp->String("<html>401 Unauthorized</html>");
            return ;
        }

        // 删除本地文件
        string filepath = "files/" + user.username + "/" + PathUtil::base(filename);
        if (unlink(filepath.c_str()) != 0 && errno != ENOENT) {
            resp->set_status(HttpStatusInternalServerError);
            resp->String("<html>500 Internal Server Error</html>");
            return ;
        }

        // 删除数据库记录
        string sql = "DELETE FROM tbl_file WHERE uid="
            + std::to_string(user.id)
            + " AND filename='" + mysql_escape(filename) + "'";

        resp->MySQL(MYSQL_URL, sql, [resp](MySQLResultCursor* cursor)
        {
            nlohmann::json json;
            if (cursor->get_cursor_status() != MYSQL_STATUS_OK) {
                json["success"] = false;
                json["error"] = "database error";
            } else {
                json["success"] = true;
            }
            resp->headers["Content-Type"] = "application/json; charset=utf-8";
            resp->String(json.dump());
        });
    });
}
