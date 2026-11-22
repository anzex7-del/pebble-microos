#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>
static char *read_all(const char *path) {
    FILE *f=fopen(path,"rb"); if(!f) { perror(path); exit(1); }
    fseek(f,0,SEEK_END); long n=ftell(f); rewind(f);
    char *s=calloc((size_t)n+1,1); if(!s) exit(1);
    if(fread(s,1,(size_t)n,f)!=(size_t)n) exit(1); fclose(f); return s;
}
int main(int argc,char **argv) {
    JSGlobalContextRef ctx=JSGlobalContextCreate(NULL);
    for(int i=1;i<argc;i++) {
        char *s=read_all(argv[i]); JSStringRef code=JSStringCreateWithUTF8CString(s); JSValueRef ex=NULL;
        JSEvaluateScript(ctx,code,NULL,NULL,1,&ex); free(s); JSStringRelease(code);
        if(ex) { JSStringRef e=JSValueToStringCopy(ctx,ex,NULL); size_t n=JSStringGetMaximumUTF8CStringSize(e); char *out=calloc(n,1); JSStringGetUTF8CString(e,out,n); fprintf(stderr,"%s: %s\n",argv[i],out); free(out); JSStringRelease(e); return 1; }
    }
    JSGlobalContextRelease(ctx); puts("PASS: browser logic and screen construction (mock DOM; no visual rendering)"); return 0;
}
