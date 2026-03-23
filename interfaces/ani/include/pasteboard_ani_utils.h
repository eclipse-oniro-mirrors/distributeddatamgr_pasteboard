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

#ifndef ANI_PASTEBOARD_UTILS_H
#define ANI_PASTEBOARD_UTILS_H

#include <ani.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

template<typename T>
class NativeObjectWrapper {
public:
    static ani_object Create([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_class clazz)
    {
        if (env == nullptr) {
            return nullptr;
        }
        T* nativePtr = new T;
        ani_object obj = Wrap(env, clazz, nativePtr);
        if (obj == nullptr) {
            delete nativePtr;
            return nullptr;
        }
        return obj;
    }

    static ani_object Wrap([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_class clazz, T* nativePtr)
    {
        if (env == nullptr || nativePtr == nullptr) {
            return nullptr;
        }
        ani_method ctor;
        if (ANI_OK != env->Class_FindMethod(clazz, "<ctor>", "l:", &ctor)) {
            ani_object nullobj = nullptr;
            return nullobj;
        }

        ani_object obj;
        if (ANI_OK != env->Object_New(clazz, ctor, &obj, reinterpret_cast<ani_long>(nativePtr))) {
            return nullptr;
        }
        return obj;
    }

    static T* Unwrap(ani_env *env, ani_object object)
    {
        if (env == nullptr) {
            return nullptr;
        }
        ani_long nativePtr;
        if (ANI_OK != env->Object_GetFieldByName_Long(object, "nativePtr", &nativePtr)) {
            return nullptr;
        }
        return reinterpret_cast<T*>(nativePtr);
    }
};

class NativeObject {
public:
    virtual ~NativeObject() = default;
};

template<typename T>
class NativeObjectAdapter : public NativeObject {
public:
    explicit NativeObjectAdapter()
    {
        obj_ = std::make_shared<T>();
    }

    std::shared_ptr<T> Get()
    {
        return obj_;
    }

private:
    std::shared_ptr<T> obj_;
};

class NativeObjectManager {
public:
    static NativeObjectManager& GetInstance()
    {
        static NativeObjectManager instance;
        return instance;
    }

    template<typename T>
    std::shared_ptr<T> Get(ani_object &object)
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);

        auto iter = aniToNativeObjMapper_.find(object);
        if (iter != aniToNativeObjMapper_.end()) {
            return std::static_pointer_cast<NativeObjectAdapter<T>>(iter->second)->Get();
        }
        auto nativeObj = std::make_shared<NativeObjectAdapter<T>>();
        aniToNativeObjMapper_.emplace(object, nativeObj);
        return nativeObj->Get();
    }

private:
    std::unordered_map<ani_object, std::shared_ptr<NativeObject>> aniToNativeObjMapper_;
    std::mutex mutex_;
};

std::string ANIUtils_ANIStringToStdString(ani_env *env, ani_string ani_str)
{
    if (env == nullptr) {
        return "";
    }
    ani_size strSize;
    env->String_GetUTF8Size(ani_str, &strSize);

    std::vector<char> buffer(strSize + 1); // +1 for null terminator
    char* utf8_buffer = buffer.data();

    ani_size bytes_written = 0;
    env->String_GetUTF8(ani_str, utf8_buffer, strSize + 1, &bytes_written);

    utf8_buffer[bytes_written] = '\0';
    std::string content = std::string(utf8_buffer);
    return content;
}

bool ANIUtils_UnionIsInstanceOf(ani_env *env, ani_object union_obj, std::string& cls_name)
{
    if (env == nullptr) {
        return false;
    }
    ani_class cls;
    env->FindClass(cls_name.c_str(), &cls);

    ani_boolean ret;
    env->Object_InstanceOf(union_obj, cls, &ret);
    return ret;
}

class UnionAccessor {
public:
    UnionAccessor(ani_env *env, ani_object &obj) : env_(env), obj_(obj)
    {
    }

    bool IsInstanceOf(const std::string& cls_name)
    {
        if (env_ == nullptr) {
            return false;
        }
        ani_class cls;
        env_->FindClass(cls_name.c_str(), &cls);

        ani_boolean ret;
        env_->Object_InstanceOf(obj_, cls, &ret);
        return ret;
    }

    template<typename T>
    bool TryConvert(T &value);

private:
    ani_env *env_;
    ani_object obj_;
};

template<>
bool UnionAccessor::TryConvert<ani_boolean>(ani_boolean &value)
{
    if (env_ == nullptr) {
        return false;
    }
    return ANI_OK == env_->Object_CallMethodByName_Boolean(obj_, "booleanValue", nullptr, &value);
}

template<>
bool UnionAccessor::TryConvert<ani_double>(ani_double &value)
{
    if (env_ == nullptr) {
        return false;
    }
    return ANI_OK == env_->Object_CallMethodByName_Double(obj_, "toDouble", nullptr, &value);
}

template<>
bool UnionAccessor::TryConvert<ani_string>(ani_string &value)
{
    value = static_cast<ani_string>(obj_);
    return true;
}

template<>
bool UnionAccessor::TryConvert<std::string>(std::string &value)
{
    value = ANIUtils_ANIStringToStdString(env_, static_cast<ani_string>(obj_));
    return true;
}

template<typename T>
class SharedPtrHolder : public NativeObject {
public:
    SharedPtrHolder(std::shared_ptr<T> &sptr) : sptr_(sptr)
    {
    }

    std::shared_ptr<T> Get()
    {
        return sptr_;
    }

    std::shared_ptr<T> GetOrDefault()
    {
        if (!sptr_) {
            sptr_ = std::make_shared<T>();
        }
        return sptr_;
    }

private:
    std::shared_ptr<T> sptr_;
};

class NativePtrCleaner {
    public:
        static void Clean([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
        {
            ani_long ptr = 0;
            if (ANI_OK != env->Object_GetFieldByName_Long(object, "targetPtr", &ptr)) {
                return;
            }

            if (ptr != 0) {
                delete reinterpret_cast<NativeObject *>(ptr);
                ptr = 0;
            }
        }

        NativePtrCleaner(ani_env *env)
            : env_(env)
        {
        }

        ani_status Bind(ani_class cls)
        {
            std::array methods = {
                ani_native_function { "clean", nullptr, reinterpret_cast<void *>(NativePtrCleaner::Clean) },
            };

            if (env_ == nullptr) {
                return (ani_status)ANI_ERROR;
            }
            if (ANI_OK != env_->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
                return (ani_status)ANI_ERROR;
            };

            return ANI_OK;
        }

    private:
        ani_env *env_ = nullptr;
};
#endif
