/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pasteboard_ani_observer.h"

#include <chrono>
#include <thread>

#include "common/pasteboard_common_utils.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

PasteboardAniObserverImpl::PasteboardAniObserverImpl(ani_env *env, ani_object callback)
    : env_(env), callback_(callback)
{
    if (env_ != nullptr && callback_ != nullptr) {
        ani_ref ref;
        if (env_->NewLocalRef(callback_, &ref) == ANI_OK) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "PasteboardAniObserverImpl created.");
        }
    }
}

PasteboardAniObserverImpl::~PasteboardAniObserverImpl()
{
    if (env_ != nullptr && callback_ != nullptr) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "PasteboardAniObserverImpl destroyed.");
    }
}

void PasteboardAniObserverImpl::OnPasteboardChanged()
{
    if (env_ == nullptr || callback_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "OnPasteboardChanged env or callback is null.");
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "OnPasteboardChanged enter.");
    ani_method method;
    static const char *invokeMethodName = "invoke";
    ani_class cls;
    if (env_->GetClassObject(callback_, &cls) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "GetClassObject failed.");
        return;
    }
    if (env_->Class_FindMethod(cls, invokeMethodName, nullptr, &method) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "FindMethod invoke failed.");
        return;
    }
    ani_boolean result = false;
    if (env_->Object_CallMethod_Boolean(callback_, method, &result) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "CallMethod invoke failed.");
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "OnPasteboardChanged callback invoked.");
}

PasteboardAniObserver::PasteboardAniObserver(std::shared_ptr<PasteboardAniObserverImpl> impl) : impl_(impl) {}

void PasteboardAniObserver::OnPasteboardChanged()
{
    if (impl_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "PasteboardAniObserver impl is null.");
        return;
    }
    impl_->OnPasteboardChanged();
}

PasteboardAniCallbackHolder::PasteboardAniCallbackHolder(ani_env *env, ani_object callback)
    : env_(env), callback_(callback)
{
    if (env_ != nullptr && callback_ != nullptr) {
        ani_ref ref;
        env_->NewLocalRef(callback_, &ref);
    }
}

PasteboardAniCallbackHolder::~PasteboardAniCallbackHolder()
{
    if (env_ != nullptr && callback_ != nullptr) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_ANI, "PasteboardAniCallbackHolder destroyed.");
    }
}

void PasteboardAniObserverManager::AddObserverInternal(ani_env *env, ani_object callback,
    std::vector<std::shared_ptr<PasteboardAniObserverImpl>> &observers, PasteboardObserverType type)
{
    if (env == nullptr || callback == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "AddObserverInternal env or callback is null.");
        return;
    }
    for (auto &observer : observers) {
        if (observer != nullptr && observer->GetCallback() == callback) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_ANI, "Observer already exists.");
            return;
        }
    }
    auto impl = std::make_shared<PasteboardAniObserverImpl>(env, callback);
    if (impl == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Create PasteboardAniObserverImpl failed.");
        return;
    }
    observers.push_back(impl);
    auto observer = std::make_shared<PasteboardAniObserver>(impl);
    if (observer == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Create PasteboardAniObserver failed.");
        return;
    }
    PasteboardClient::GetInstance()->AddObserver(observer, type);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "AddObserverInternal success, type=%{public}d.",
        static_cast<int>(type));
}

void PasteboardAniObserverManager::RemoveObserverInternal(ani_env *env, ani_object callback,
    std::vector<std::shared_ptr<PasteboardAniObserverImpl>> &observers, PasteboardObserverType type)
{
    if (env == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "RemoveObserverInternal env is null.");
        return;
    }
    if (callback == nullptr) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "Remove all observers of type=%{public}d.",
            static_cast<int>(type));
        observers.clear();
        return;
    }
    for (auto it = observers.begin(); it != observers.end(); ++it) {
        if (*it != nullptr && (*it)->GetCallback() == callback) {
            observers.erase(it);
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "RemoveObserverInternal success, type=%{public}d.",
                static_cast<int>(type));
            return;
        }
    }
    PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_ANI, "Observer not found, type=%{public}d.",
        static_cast<int>(type));
}

void PasteboardAniObserverManager::AddLocalObserver(ani_env *env, ani_object callback)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    AddObserverInternal(env, callback, localObservers_, PasteboardObserverType::OBSERVER_LOCAL);
}

void PasteboardAniObserverManager::RemoveLocalObserver(ani_env *env, ani_object callback)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    RemoveObserverInternal(env, callback, localObservers_, PasteboardObserverType::OBSERVER_LOCAL);
}

void PasteboardAniObserverManager::RemoveAllLocalObservers()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    localObservers_.clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "RemoveAllLocalObservers success.");
}

void PasteboardAniObserverManager::AddRemoteObserver(ani_env *env, ani_object callback)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    AddObserverInternal(env, callback, remoteObservers_, PasteboardObserverType::OBSERVER_REMOTE);
}

void PasteboardAniObserverManager::RemoveRemoteObserver(ani_env *env, ani_object callback)
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    RemoveObserverInternal(env, callback, remoteObservers_, PasteboardObserverType::OBSERVER_REMOTE);
}

void PasteboardAniObserverManager::RemoveAllRemoteObservers()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    remoteObservers_.clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "RemoveAllRemoteObservers success.");
}

size_t PasteboardAniObserverManager::GetLocalObserverCount()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return localObservers_.size();
}

size_t PasteboardAniObserverManager::GetRemoteObserverCount()
{
    std::lock_guard<std::mutex> lockGuard(mutex_);
    return remoteObservers_.size();
}

void PasteboardAniEventHandler::PostEvent(std::function<void()> &&task)
{
    if (task == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "PostEvent task is null.");
        return;
    }
    std::thread thread([task = std::move(task)]() {
        task();
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniPostEvent");
    thread.detach();
}

void PasteboardAniEventHandler::PostDelayedEvent(std::function<void()> &&task, uint64_t delayMs)
{
    if (task == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "PostDelayedEvent task is null.");
        return;
    }
    std::thread thread([task = std::move(task), delayMs]() {
        if (delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
        task();
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniPostDelayedEvent");
    thread.detach();
}

} // namespace MiscServices
} // namespace OHOS
