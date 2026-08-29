/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
#include "loader.h"

#include <dlfcn.h>
#include <fstream>

#include "pasteboard_hilog.h"
namespace OHOS::MiscServices {
Loader::Loader() {}

Loader::~Loader() {}

void Loader::LoadComponents()
{
    Config config = LoadConfig();
    for (auto &component : config.components) {
        if (component.lib.empty()) {
            continue;
        }
        if (Loader::ComponentIsExist(component.lib)) {
            continue;
        }
        // no need to close the component, so we don't keep the handles
        auto handle = dlopen(component.lib.c_str(), RTLD_LAZY);
        if (handle == nullptr) {
            PASTEBOARD_HILOGE(
                PASTEBOARD_MODULE_SERVICE, "dlopen(%{public}s) failed(%{public}d)!", component.lib.c_str(), errno);
            continue;
        }
        Loader::handleMap.insert(std::pair(component.lib, handle));

        if (component.constructor.empty()) {
            continue;
        }

        auto ctor = reinterpret_cast<Constructor>(dlsym(handle, component.constructor.c_str()));
        if (ctor == nullptr) {
            continue;
        }
        ctor(component.params.c_str());
    }
}

int32_t Loader::LoadUid()
{
    Config config = LoadConfig();
    return config.uid;
}

Config Loader::LoadConfig()
{
    Config config;
    std::string context;
    std::ifstream fin(CONF_FILE);
    while (fin.good()) {
        std::string line;
        std::getline(fin, line);
        context += line;
    }
    config.Unmarshall(context);
    return config;
}

bool Loader::ComponentIsExist(const std::string &lib)
{
    return Loader::handleMap.find(lib) != Loader::handleMap.end();
}
} // namespace OHOS::MiscServices