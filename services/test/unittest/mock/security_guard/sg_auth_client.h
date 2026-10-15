#ifndef SECURITY_GUARD_SG_AUTH_CLIENT_H
#define SECURITY_GUARD_SG_AUTH_CLIENT_H

#include <functional>
#include <string>
#include <vector>

namespace OHOS::Security::SecurityGuard {

// 阻断策略枚举
enum AuthStrategy {
    // 允许通过
    ALLOW = 0,
    // 拒绝阻断
    DENY = 1,
    // 超时
    TIMEOUT = 2,
};

// 客户端会话信息
struct AuthClientInfo {
    // 客户端会话唯一标识
    std::string clientId;
    // 应用包名
    std::string bundleName;
    // 客户端订阅的事件类型ID列表
    std::vector<int> subscribedEvents;
    // 超时默认阻断策略
    AuthStrategy defaultTimeoutStrategy;
    // 客户端响应超时时间
    uint32_t defaultTimeoutMs;
};

// 单个客户端阻断结果
struct ClientAuthResult {
    // 客户端会话唯一标识
    std::string clientId;
    // 阻断策略
    AuthStrategy authStrategy;
};

// 所有客户端阻断结果
struct AuthResult {
    // 所有客户端汇聚后的阻断策略
    AuthStrategy mergedAuthStrategy;
    // 客户端阻断结果列表
    std::vector<ClientAuthResult> clientAuthResult;
};

// 阻断事件信息
struct AuthEventInfo {
    // 事件类型ID
    int eventId;
    // 事件版本号
    std::string version;
    // 事件内容，JSON字符串格式
    std::string content;
};

// Provider 回调接口，在 CreateAuthProvider 时传入
class AuthProviderCallback {
public:
    virtual ~AuthProviderCallback() = default;
    // 关联的客户端会话状态变更时回调
    virtual void OnClientStatusChanged(const std::vector<AuthClientInfo> &clientInfos) = 0;
};

// 阻断结果回调
using AuthResultCallback = std::function<void(const AuthResult &result)>;

// AuthProvider 接口，由 NewAuthProvider 工厂函数创建
class AuthProvider {
public:
    virtual ~AuthProvider() = default;

    /**
     * 请求事件的阻断结果，通过回调返回
     * @param event - 阻断事件信息
     * @param callback - 阻断结果回调
     */
    virtual int32_t RequestAuthResult(const AuthEventInfo &event, const AuthResultCallback &callback) = 0;
};

/**
 * 创建 AuthProvider 会话
 * 同一进程最多1个Provider实例，全局最多16个
 * 成功后OnClientStatusChanged回调可收到已有客户端的信息
 *
 * @permission ohos.permission.AUTH_SERVICE_ACCESS
 * @param callback - Provider回调接口，含策略接收和客户端状态变更通知
 * @param serviceId - 服务标识，标识本Provider提供的阻断服务类型
 * @param provider - 创建的Provider实例，失败时返回nullptr
 */
int32_t CreateAuthProvider(const AuthProviderCallback *callback, const std::string &serviceId,
    std::unique_ptr<AuthProvider> &provider);

} // namespace OHOS::Security::SecurityGuard

#endif // SECURITY_GUARD_SG_AUTH_CLIENT_H
