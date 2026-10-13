#include <windows.h>
#include <stdio.h>

__declspec(dllimport) HRESULT WINAPI DirectDrawCreate(void *, void **, void *);

int main(void)
{
    HMODULE module;
    char path[MAX_PATH];
    FARPROC imported;
    DWORD length;

    module = GetModuleHandleA("ddraw.dll");
    imported = (FARPROC)DirectDrawCreate;
    path[0] = '\0';
    length = module ? GetModuleFileNameA(module, path, sizeof(path)) : 0;
    printf("ARCH=x86\nMODULE=%s\nMODULE_PATH_LENGTH=%lu\nIMPORT_SLOT=%p\n",
           path, (unsigned long)length, imported);
    /* The static import is inspected but intentionally never invoked. */
    return (module != NULL && length != 0 && imported != NULL) ? 0 : 17;
}
