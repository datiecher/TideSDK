/**
* This file has been modified from its orginal sources.
*
* Copyright (c) 2012 Software in the Public Interest Inc (SPI)
* Copyright (c) 2012 David Pratt
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
***
* Copyright (c) 2008-2012 Appcelerator Inc.
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#include "javascript_module.h"

namespace tide
{
    KKJSMethod::KKJSMethod(JSContextRef context, JSObjectRef jsobject, JSObjectRef thisObject) :
        KMethod("JavaScript.KKJSMethod"),
        context(NULL),
        jsobject(jsobject),
        thisObject(thisObject)
    {

        /* KJS methods run in the global context that they originated from
         * this seems to prevent nasty crashes from trying to access invalid
         * contexts later. Global contexts need to be registered by all modules
         * that use a KJS context. */
        JSObjectRef globalObject = JSContextGetGlobalObject(context);
        JSGlobalContextRef globalContext = KJSUtil::GetGlobalContext(globalObject);

        // This context hasn't been registered. Something has gone pretty
        // terribly wrong and Kroll will likely crash soon. Nonetheless, keep
        // the user up-to-date to keep their hopes up.
        if (globalContext == NULL)
            std::cerr << "Could not locate global context for a KJS method."  <<
                " One of the modules is misbehaving." << std::endl;

        this->context = globalContext;

        KJSUtil::ProtectGlobalContext(this->context);
        JSValueProtect(this->context, jsobject);
        if (thisObject != NULL)
            JSValueProtect(this->context, thisObject);

        this->kobject = new KKJSObject(this->context, jsobject);
    }

    KKJSMethod::~KKJSMethod()
    {
        JSValueUnprotect(this->context, this->jsobject);
        if (this->thisObject != NULL)
            JSValueUnprotect(this->context, this->thisObject);

        KJSUtil::UnprotectGlobalContext(this->context);
    }

    KValueRef KKJSMethod::Get(const char *name)
    {
        return kobject->Get(name);
    }

    void KKJSMethod::Set(const char *name, KValueRef value)
    {
        return kobject->Set(name, value);
    }

    bool KKJSMethod::Equals(KObjectRef other)
    {
        return this->kobject->Equals(other);
    }

    SharedStringList KKJSMethod::GetPropertyNames()
    {
        return kobject->GetPropertyNames();
    }

    bool KKJSMethod::HasProperty(const char* name)
    {
        return kobject->HasProperty(name);
    }

    bool KKJSMethod::SameContextGroup(JSContextRef c)
    {
        return kobject->SameContextGroup(c);
    }

    JSObjectRef KKJSMethod::GetJSObject()
    {
        return this->jsobject;
    }

    KValueRef KKJSMethod::Call(JSObjectRef thisObject, const ValueList& args)
    {
        JSValueRef* jsArgs = new JSValueRef[args.size()];
        for (int i = 0; i < (int) args.size(); i++)
        {
            KValueRef arg = args.at(i);
            jsArgs[i] = KJSUtil::ToJSValue(arg, this->context);
        }

        JSValueRef exception = NULL;
        JSValueRef jsValue = JSObjectCallAsFunction(this->context, thisObject,
            this->thisObject, args.size(), jsArgs, &exception);

        delete [] jsArgs; // clean up args

        if (jsValue == NULL && exception != NULL) //exception thrown
        {
            KValueRef exceptionValue = KJSUtil::ToKrollValue(exception, this->context, NULL);
            throw ValueException(exceptionValue);
        }

        return KJSUtil::ToKrollValue(jsValue, this->context, NULL);
    }

    KValueRef KKJSMethod::Call(const ValueList& args)
    {
        return this->Call(this->jsobject, args);
    }

    KValueRef KKJSMethod::Call(KObjectRef thisObject, const ValueList& args)
    {
        JSValueRef thisObjectValue = KJSUtil::ToJSValue(Value::NewObject(thisObject), this->context);
        if (!JSValueIsObject(this->context, thisObjectValue))
        {
            SharedString ss(thisObject->DisplayString());
            throw ValueException::FromFormat("Could not convert %s to JSObjectRef for KKJSMethod::Call",
                ss->c_str());
        }

        JSObjectRef jsThisObject = JSValueToObject(this->context, thisObjectValue, NULL);
        if (!jsThisObject)
        {
            SharedString ss(thisObject->DisplayString());
            throw ValueException::FromFormat("Could not convert %s to JSObjectRef for KKJSMethod::Call",
                ss->c_str());
        }

        return this->Call(jsThisObject, args);
    }
}
