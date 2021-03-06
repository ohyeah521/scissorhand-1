
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <dlfcn.h>
#include <openssl/evp.h>
// #include <sqlite3.h>

#include "elf_hooker.h"
#include "inlinehook.h"

static int (*__old_impl_fopen)(const char *pathname, int flags);

static int (*__old_impl_sqlite_open)(const char* name, void **ppDb);

static void (*__old_impl__ZN7cocos2d5CCLogEPKcz)(const char* fmt, ...);

extern "C" {

    static int __nativehook_impl_fopen(const char *pathname, int flags)
    {
        log_info("__nativehook_impl_fopen -> (%s)\n", pathname);
        return __old_impl_fopen(pathname, flags);
    }

    static int __nativehook_impl_sqlite_open(const char* name, void **ppDb)
    {
        log_info("__nativehook_impl_sqlite_open -> (%s)\n", name);
        return __old_impl_sqlite_open(name, ppDb);
    }

    static void __nativehook_impl__ZN7cocos2d5CCLogEPKcz(const char* fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        log_info(fmt, ap);
        va_end(ap);
    }
}

static bool __prehook(const char* module_name, const char* func_name)
{
    if (strstr(module_name, "libgame.so") != NULL)
    {
       return true;
    }
    return false;
}

#if (ELFHOOK_STANDALONE)

int main(int argc, char* argv[])
{
    char ch = 0;
    elf_hooker hooker;

    void* h = dlopen("libart.so", RTLD_LAZY);
    void* f = dlsym(h,"artAllocObjectFromCodeResolvedRegion");
    log_info("artAllocObjectFromCodeResolvedRegion : %p\n", f);

    hooker.set_prehook_cb(__prehook);
    hooker.phrase_proc_maps();
    hooker.dump_module_list();
    // hooker.hook_all_modules("dlopen", (void*)__nativehook_impl_dlopen, (void**)&__old_impl_dlopen);
    // hooker.hook_all_modules("connect", (void*)__nativehook_impl_connect, (void**)&__old_impl_connect);

    do {
        ch = getc(stdin);
    } while(ch != 'q');
    return 0;
}

#else

#include <jni.h>

static char* __class_name = "com/wadahana/testhook/ElfHooker";
static elf_hooker __hooker;
static JavaVM* __java_vm = NULL;
static bool __is_attached = false;

static JNIEnv* __getEnv(bool* attached);
static void __releaseEnv(bool attached);
static int __set_hook(JNIEnv *env, jobject thiz);
static int __test(JNIEnv *env, jobject thiz);
static int __elfhooker_init(JavaVM* vm, JNIEnv* env);
static void __elfhooker_deinit(void);

static JNINativeMethod __methods[] =
{
    {"setHook","()I",(void *)__set_hook },
    {"test","()I",(void *)__test },
};

static int __set_hook(JNIEnv *env, jobject thiz)
{
    log_info("__set_hook() -->\r\n");
//    __hooker.set_prehook_cb(__prehook);
    __hooker.phrase_proc_maps();
    __hooker.dump_module_list();
    __hooker.dump_proc_maps();
    // __hooker.hook_all_modules("dlopen", (void*)__nativehook_impl_dlopen, (void**)&__old_impl_dlopen);
    // __hooker.hook_all_modules("connect", (void*)__nativehook_impl_connect, (void**)&__old_impl_connect);
    // __hooker.hook_all_modules("android_dlopen_ext", (void*)__nativehook_impl_android_dlopen_ext, (void**)&__old_impl_android_dlopen_ext);

    // __hooker.hook_all_modules("EVP_CipherInit", (void*)__nativehook_impl_evp_cipherinit, (void**)&__old_impl_evp_cipherinit);

#if 0
    void* h = dlopen("libart.so", RTLD_LAZY);
    if (h != NULL) {
        void* f = dlsym(h,"artAllocObjectFromCodeResolvedRegion");
        log_info("artAllocObjectFromCodeResolvedRegion : %p\n", f);
    } else {
        log_error("open libart.so fail\n");
    }
#endif
    return 0;
}

static int __test(JNIEnv *env, jobject thiz)
{
    log_info("__test() -->\r\n");
//    __hooker.dump_proc_maps();
    return 0;
}

static int __elfhooker_register_native_methods(JNIEnv* env, const char* class_name,
                                JNINativeMethod* methods, int num_methods)
{

    log_info("RegisterNatives start for \'%s\'", __class_name);

    jclass clazz = env->FindClass(class_name);
    if (clazz == NULL)
    {
        log_error("Native registration unable to find class \'%s\'", class_name);
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, methods, num_methods) < 0)
    {
        log_error("RegisterNatives failed for \'%s\'", class_name );
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

static int __elfhooker_init(JavaVM* vm, JNIEnv* env)
{
    log_info("hookwrapper_init() -->\r\n");
    if (!__elfhooker_register_native_methods(env, __class_name,
                __methods, sizeof(__methods) / sizeof(__methods[0])))
    {
        log_error("register hookJNIMethod fail, \r\n");
        __elfhooker_deinit();
        return -2;
    }

  return 0;
}

static void __elfhooker_deinit(void)
{
    log_info("hookwrapper_deinit()->\r\n");
    return;
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    bool attached;
    __java_vm = vm;

    if ((env = __getEnv(&__is_attached)) == NULL)
    {
        log_error("getEnv fail\r\n");
        return -1;
    }
    assert(!__is_attached);
    if (__elfhooker_init(vm, env) < 0)
    {
        log_error("__elfhooker_init fail\r\n");
        return -1;
    }
    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved)
{
    bool attached;
    JNIEnv* env = __getEnv(&__is_attached);
    assert(!__is_attached);

    __elfhooker_deinit();
    return ;
}

static JNIEnv* __getEnv(bool* attached)
{
    JNIEnv* env = NULL;
    *attached = false;
    int ret = __java_vm->GetEnv((void**)&env, JNI_VERSION_1_4);
    if (ret == JNI_EDETACHED)
    {
        if (0 != __java_vm->AttachCurrentThread(&env, NULL)) {
            return NULL;
        }
        *attached = true;
        return env;
    }

    if (ret != JNI_OK) {
        return NULL;
    }

    return env;
}

static void __releaseEnv(bool attached)
{
    if (attached)
        __java_vm->DetachCurrentThread();
}

void __attribute__ ((constructor)) libElfHook_main()
{
    log_info(".............................loaded\r\n");

    elf_hooker hooker;

    // void* h = dlopen("libart.so", RTLD_LAZY);
    // void* f = dlsym(h,"artAllocObjectFromCodeResolvedRegion");
    // log_info("artAllocObjectFromCodeResolvedRegion : %p\n", f);

    hooker.set_prehook_cb(__prehook);
    hooker.phrase_proc_maps();

    // hooker.hook_all_modules("fopen", (void*)__nativehook_impl_fopen, (void**)&__old_impl_fopen);
    hooker.hook_all_modules("sqlite3_open", (void*)__nativehook_impl_sqlite_open, (void**)&__old_impl_sqlite_open);
    // hooker.hook_all_modules("_ZN7cocos2d5CCLogEPKcz", (void*)__nativehook_impl__ZN7cocos2d5CCLogEPKcz, (void**)&__old_impl__ZN7cocos2d5CCLogEPKcz);
}
#endif
