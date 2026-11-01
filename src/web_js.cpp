// Bounded Duktape ES5 bridge for basic inline DOM updates.
// This is not a complete browser DOM or a promise of modern JS compatibility.
#include "web_js.h"
#include "native_document.h"
#ifdef X360BROWSER_HOST_TEST
#include <windows.h>
#else
#include <xtl.h>
#endif
#include "../vendor/duktape/duktape.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

struct BrowserJSQuota {size_t used,limit;DWORD deadline;};
struct BrowserJSChunk{std::string id,value;bool text;};
struct BrowserJSState{
    std::string writes;std::vector<BrowserJSChunk> edits;
    BrowserJSState(){}
};
static void* dukLimitAlloc(void* udata,duk_size_t size){
    BrowserJSQuota* q=(BrowserJSQuota*)udata;
    if(!size || size>q->limit-sizeof(size_t) || q->used>q->limit-size-sizeof(size_t))return NULL;
    size_t* base=(size_t*)malloc(size+sizeof(size_t));
    if(!base)return NULL;
    base[0]=size;q->used+=size+sizeof(size_t);return base+1;
}
static void dukLimitFree(void* udata,void* p){
    if(!p)return;BrowserJSQuota* q=(BrowserJSQuota*)udata;
    size_t* base=(size_t*)p-1;
    if(q->used>=base[0]+sizeof(size_t))q->used-=base[0]+sizeof(size_t);
    free(base);
}
static void* dukLimitRealloc(void* udata,void* p,duk_size_t size){
    if(!p)return dukLimitAlloc(udata,size);
    if(!size){dukLimitFree(udata,p);return NULL;}
    BrowserJSQuota* q=(BrowserJSQuota*)udata;size_t* base=(size_t*)p-1;
    size_t previous=base[0];
    if(size>q->limit-sizeof(size_t) || q->used-previous>q->limit-size)return NULL;
    size_t* next=(size_t*)realloc(base,size+sizeof(size_t));
    if(!next)return NULL;
    next[0]=size;q->used=q->used-previous+size;return next+1;
}
extern "C" int browserJSDeadlineExpired(void* opaque){
    BrowserJSQuota* q=(BrowserJSQuota*)opaque;
    return (int32_t)(GetTickCount()-q->deadline)>0;
}
static void dukLimitFatal(void*,const char*){abort();}
static BrowserJSState* activeState(duk_context* ctx){
    if(!duk_get_global_string(ctx,"__browserState"))return NULL;
    BrowserJSState* result=(BrowserJSState*)duk_get_pointer(ctx,-1);duk_pop(ctx);return result;
}
static duk_ret_t jsWrite(duk_context* ctx){
    BrowserJSState* state=activeState(ctx);if(!state)return 0;
    const char* value=duk_safe_to_string(ctx,0);size_t bytes=strlen(value);
    if(bytes<65536 && state->writes.size()+bytes<65536)state->writes+=value;
    return 0;
}
static duk_ret_t jsSet(duk_context* ctx){
    BrowserJSState* state=activeState(ctx);if(!state||state->edits.size()>64)return 0;
    const char* id=duk_safe_to_string(ctx,0);
    const char* value=duk_safe_to_string(ctx,1);
    if(strlen(id)>0 && strlen(id)<128 && strlen(value)<16384){
        BrowserJSChunk chunk;chunk.id=id;chunk.value=value;chunk.text=duk_get_boolean(ctx,2)!=0;
        state->edits.push_back(chunk);
    }
    return 0;
}
static bool asciiCI(const std::string& html,size_t at,const char* pattern){
    for(size_t i=0;pattern[i];++i){
        if(at+i>=html.size())return false;
        char a=html[at+i],b=pattern[i];
        if(a>='A'&&a<='Z')a=(char)(a+32);
        if(b>='A'&&b<='Z')b=(char)(b+32);
        if(a!=b)return false;
    }
    return true;
}
static size_t findTag(const std::string& html,const char* name,size_t pos){
    while(pos<html.size()){pos=html.find('<',pos);if(pos==std::string::npos)return pos;
        if(asciiCI(html,pos+1,name)){size_t e=pos+1+strlen(name);
            if(e<html.size()&&(html[e]=='>'||html[e]=='/'||html[e]==' '||html[e]=='\t'||html[e]=='\n'))return pos;
        }
        ++pos;
    }
    return std::string::npos;
}
static std::string htmlEscaped(const std::string& s){
    std::string result;
    for(size_t i=0;i<s.size();++i){
        switch(s[i]){case '&':result+="&amp;";break;case '<':result+="&lt;";break;case '>':result+="&gt;";break;case '"':result+="&quot;";break;default:result+=s[i];}
    }
    return result;
}
static void applyEdit(std::string& html,const BrowserJSChunk& edit){
    size_t where=0;
    while((where=html.find("id=",where))!=std::string::npos){
        size_t start=where+3;if(start>=html.size())return;
        char quote=html[start];if(quote=='"'||quote=='\'')++start;else quote=' ';
        if(html.compare(start,edit.id.size(),edit.id)!=0){where=start;continue;}
        size_t end=start+edit.id.size();
        if(quote==' ' && end<html.size() && html[end]!=' '&&html[end]!='>'){where=end;continue;}
        if(quote!=' ' && (end>=html.size()||html[end]!=quote)){where=end;continue;}
        size_t tagStart=html.rfind('<',where);size_t openEnd=html.find('>',where);
        if(tagStart==std::string::npos||openEnd==std::string::npos)return;
        if(html.find('>',tagStart)<where){where=end;continue;}
        size_t tagName=tagStart+1,tagEnd=tagName;
        while(tagEnd<html.size() && ((html[tagEnd]>='a'&&html[tagEnd]<='z')||(html[tagEnd]>='A'&&html[tagEnd]<='Z')||html[tagEnd]=='-'))++tagEnd;
        if(tagEnd==tagName)return;
        std::string close="</"+html.substr(tagName,tagEnd-tagName)+">";
        size_t closeAt=html.find(close,openEnd+1);
        if(closeAt==std::string::npos)return;
        html.replace(openEnd+1,closeAt-openEnd-1,edit.text?htmlEscaped(edit.value):edit.value);
        return;
    }
}
static const char* bootstrap =
    "var window=this;var globalThis=this;"
    "var document={write:__write,writeln:__write,getElementById:function(id){"
    " var obj={};Object.defineProperty(obj,'innerHTML',{set:function(v){__set(id,String(v),false);}});"
    " Object.defineProperty(obj,'textContent',{set:function(v){__set(id,String(v),true);}});return obj;},"
    "querySelector:function(s){return s.charAt(0)==='#'?this.getElementById(s.substr(1)):null;}};"
    "var console={log:function(){},warn:function(){},error:function(){}};";
bool browserJSPreprocess(const char* html,const char* baseURL,std::string& output,int* ran,int* failed,BrowserJSFetch fetch,void* user){
    if(ran)*ran=0;if(failed)*failed=0;
    if(!html)return false;
    size_t bytes=strlen(html);
    if(bytes>1024*1024)return false;
    output.assign(html,bytes);
    if(output.find("<script")==std::string::npos && output.find("<SCRIPT")==std::string::npos)return true;
    BrowserJSQuota quota={0,6u*1024u*1024u,GetTickCount()+300};
    duk_context* ctx=duk_create_heap(dukLimitAlloc,dukLimitRealloc,dukLimitFree,&quota,dukLimitFatal);
    if(!ctx)return true;
    BrowserJSState state;
    duk_push_pointer(ctx,&state);duk_put_global_string(ctx,"__browserState");
    duk_push_c_function(ctx,jsWrite,1);duk_put_global_string(ctx,"__write");
    duk_push_c_function(ctx,jsSet,3);duk_put_global_string(ctx,"__set");
    duk_peval_string(ctx,bootstrap);duk_pop(ctx);
    size_t pos=0;int scripts=0,externalLoaded=0;
    while(scripts<24 && GetTickCount()<quota.deadline){
        size_t open=findTag(output,"script",pos);if(open==std::string::npos)break;
        size_t headerEnd=output.find('>',open+7);if(headerEnd==std::string::npos)break;
        size_t end=headerEnd+1;
        while(end<output.size()&&!asciiCI(output,end,"</script"))++end;
        if(end>=output.size())break;
        size_t closeEnd=output.find('>',end);if(closeEnd==std::string::npos)break;
        std::string attrs=output.substr(open,headerEnd-open);
        std::string script=output.substr(headerEnd+1,end-headerEnd-1);
        bool module=attrs.find("module")!=std::string::npos ||
          attrs.find("application/")!=std::string::npos;
        size_t srcPos=attrs.find("src=");
        bool external=srcPos!=std::string::npos;
        if(external && !module && fetch && baseURL && externalLoaded<3){
            size_t begin=srcPos+4;
            while(begin<attrs.size() && attrs[begin]==' ')++begin;
            char quote=begin<attrs.size()?attrs[begin]:0;
            if(quote=='"'||quote=='\'')++begin;
            size_t finish=begin;
            while(finish<attrs.size() && ((quote=='"'||quote=='\'')?attrs[finish]!=quote:attrs[finish]!=' '))++finish;
            std::string relative=attrs.substr(begin,finish-begin);
            char resolved[WEB_URL];
            if(relative.size()<WEB_URL && webResolveURL(baseURL,relative.c_str(),resolved,sizeof(resolved))){
                std::string b(baseURL),u(resolved);
                size_t bHost=b.find('/',8),uHost=u.find('/',8);
                if(b.substr(8,bHost==std::string::npos?b.size()-8:bHost-8)==
                   u.substr(8,uHost==std::string::npos?u.size()-8:uHost-8)){
                    unsigned char* fetched=NULL;int n=0;char finalURL[WEB_URL]="";
                    if(fetch(user,resolved,&fetched,&n,finalURL,sizeof(finalURL)) && fetched && n>0 && n<131072){
                        script.assign((const char*)fetched,n);
                        quota.deadline=GetTickCount()+300;
                        ++externalLoaded;
                    }
                    free(fetched);
                }
            }
        }
        bool execute=!module && (!external || (external && !script.empty())) &&
                     script.size()<131072;
        if(execute){
            state.writes.clear();size_t before=state.edits.size();
            int result=duk_peval_lstring(ctx,script.c_str(),script.size());
            if(result!=0){if(failed)++*failed;
#ifdef X360BROWSER_HOST_TEST
                printf("JS script error: %.140s\n",duk_safe_to_string(ctx,-1));
#endif
            }
            else if(ran)++*ran;
            duk_pop(ctx);
            if(!state.writes.empty()){
                output.insert(closeEnd+1,state.writes);
                closeEnd+=state.writes.size();
            }
            if(state.edits.size()!=before){
                for(size_t i=before;i<state.edits.size();++i)applyEdit(output,state.edits[i]);
            }
        }
        pos=closeEnd+1;++scripts;
        if(pos>=output.size()||output.size()>1024*1024)break;
    }
    duk_destroy_heap(ctx);
    return true;
}
