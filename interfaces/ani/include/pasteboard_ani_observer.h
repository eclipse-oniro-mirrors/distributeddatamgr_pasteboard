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

#ifndef PASTEBOARD_ANI_OBSERVER_H
#define PASTEBOARD_ANI_OBSERVER_H

#include <ani.h>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "pasteboard_observer.h"

namespace OHOS {
namespace MiscServices {

class PasteboardAniObserverImpl : public std::enable_shared_from_this<PasteboardAniObserverImpl> {
public:
    explicit PasteboardAniObserverImpl(ani_env *env, ani_object callback);
    ~PasteboardAniObserverImpl();
    void OnPasteboardChanged();

    ani_env *GetEnv()
    {
        return env_;
    }

    ani_object GetCallback()
    {
        return callback_;
    }

private:
    ani_env *env_ = nullptr;
    ani_object callback_ = nullptr;
};

class PasteboardAniObserver : public PasteboardObserver {
public:
    explicit PasteboardAniObserver(std::shared_ptr<PasteboardAniObserverImpl> impl);
    ~PasteboardAniObserver() override = default;
    void OnPasteboardChanged() override;

private:
    std::shared_ptr<PasteboardAniObserverImpl> impl_ = nullptr;
};

class PasteboardAniObserverManager {
public:
    static PasteboardAniObserverManager &GetInstance()
    {
        static PasteboardAniObserverManager instance;
        return instance;
    }

    void AddLocalObserver(ani_env *env, ani_object callback);
    void RemoveLocalObserver(ani_env *env, ani_object callback);
    void RemoveAllLocalObservers();

    void AddRemoteObserver(ani_env *env, ani_object callback);
    void RemoveRemoteObserver(ani_env *env, ani_object callback);
    void RemoveAllRemoteObservers();

    size_t GetLocalObserverCount();
    size_t GetRemoteObserverCount();

private:
    PasteboardAniObserverManager() = default;
    ~PasteboardAniObserverManager() = default;
    PasteboardAniObserverManager(const PasteboardAniObserverManager &) = delete;
    PasteboardAniObserverManager &operator=(const PasteboardAniObserverManager &) = delete;

    void AddObserverInternal(ani_env *env, ani_object callback,
        std::vector<std::shared_ptr<PasteboardAniObserverImpl>> &observers,
        PasteboardObserverType type);
    void RemoveObserverInternal(ani_env *env, ani_object callback,
        std::vector<std::shared_ptr<PasteboardAniObserverImpl>> &observers,
        PasteboardObserverType type);

    std::vector<std::shared_ptr<PasteboardAniObserverImpl>> localObservers_;
    std::vector<std::shared_ptr<PasteboardAniObserverImpl>> remoteObservers_;
    std::mutex mutex_;
};

class PasteboardAniCallbackHolder {
public:
    explicit PasteboardAniCallbackHolder(ani_env *env, ani_object callback);
    ~PasteboardAniCallbackHolder();

    ani_env *GetEnv()
    {
        return env_;
    }

    ani_object GetCallback()
    {
        return callback_;
    }

    bool IsValid()
    {
        return env_ != nullptr && callback_ != nullptr;
    }

private:
    ani_env *env_ = nullptr;
    ani_object callback_ = nullptr;
};

class PasteboardAniEventHandler {
public:
    static PasteboardAniEventHandler &GetInstance()
    {
        static PasteboardAniEventHandler instance;
        return instance;
    }

    void PostEvent(std::function<void()> &&task);
    void PostDelayedEvent(std::function<void()> &&task, uint64_t delayMs);

private:
    PasteboardAniEventHandler() = default;
    ~PasteboardAniEventHandler() = default;
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_ANI_OBSERVER_H
