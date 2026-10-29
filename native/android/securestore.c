// <securestore> on Android: the externs Eskiu calls, into EskiuSecureStore (Java) through JNI.
#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include "ui_native.h"

static jclass store_class(JNIEnv *env) { return ui_find_class("org.eskiu.securestore.EskiuSecureStore"); }

char *secure_native_get(const char *key) {
    JNIEnv *env = ui_jni_env();
    jclass type = store_class(env);
    if (!type) return NULL;
    jmethodID method = (*env)->GetStaticMethodID(env, type, "get", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring k = (*env)->NewStringUTF(env, key);
    jstring value = (jstring)(*env)->CallStaticObjectMethod(env, type, method, k);
    char *out = NULL;
    if (value) {
        const char *text = (*env)->GetStringUTFChars(env, value, NULL);
        out = strdup(text);
        (*env)->ReleaseStringUTFChars(env, value, text);
        (*env)->DeleteLocalRef(env, value);
    }
    (*env)->DeleteLocalRef(env, k);
    (*env)->DeleteLocalRef(env, type);
    return out;
}

static int call_bool(const char *name, const char *signature, const char *key, const char *value) {
    JNIEnv *env = ui_jni_env();
    jclass type = store_class(env);
    if (!type) return 0;
    jmethodID method = (*env)->GetStaticMethodID(env, type, name, signature);
    jstring k = (*env)->NewStringUTF(env, key);
    jstring v = value ? (*env)->NewStringUTF(env, value) : NULL;
    jboolean ok = v ? (*env)->CallStaticBooleanMethod(env, type, method, k, v) : (*env)->CallStaticBooleanMethod(env, type, method, k);
    if (v) (*env)->DeleteLocalRef(env, v);
    (*env)->DeleteLocalRef(env, k);
    (*env)->DeleteLocalRef(env, type);
    return ok ? 1 : 0;
}

int secure_native_set(const char *key, const char *value) {
    return call_bool("set", "(Ljava/lang/String;Ljava/lang/String;)Z", key, value);
}

int secure_native_remove(const char *key) { return call_bool("remove", "(Ljava/lang/String;)Z", key, NULL); }

int secure_native_is_secure(void) { return 1; }
