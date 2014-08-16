#include "kendynet.h"
#include "lua_util.h"
#include "luasocket.h"
#include "kn_timer.h"
#include "kn_time.h"

__thread engine_t g_engine = NULL;

static void sig_int(int sig){
	kn_stop_engine(g_engine);
}

static int  cb_timer(kn_timer_t timer)
{
	luaFuncRef_t *Schedule = (luaFuncRef_t*)kn_timer_getud(timer);
	const char *err;
	if((err = CallLuaFuncRef0(NULL,*Schedule,0))){
		printf("lua error0:%s\n",err);
	}
	return 1;
}

static int  lua_stop(lua_State *L){
	kn_stop_engine(g_engine);
	return 0;
}

static int  lua_getsystick(lua_State *L){
	lua_pushnumber(L,kn_systemms());
	return 1;
}


static void start(lua_State *L,const char *start_file)
{
	printf("start\n");
	char buf[4096];
	snprintf(buf,4096,"\
		local Sche = require \"lua/sche\"\
		local main,err= loadfile(\"%s\")\
		if err then print(err) end\
		Sche.Spawn(function () main() end)\
		Sche.Schedule() return Sche.Schedule",start_file);
	    luaL_loadstring(L,buf);
	if(0 != lua_pcall(L,0,1,0)){
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}
	
	luaFuncRef_t *Schedule = calloc(1,sizeof(*Schedule)); 
	*Schedule = create_luaFuncRef(L,-1);	
	kn_reg_timer(g_engine,1,cb_timer,Schedule);	
	kn_engine_run(g_engine);		
}

int main(int argc,char **argv)
{
	if(argc < 2){
		printf("usage distrilua luafile\n");
		return 0;
	}
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
    signal(SIGINT,sig_int);
    signal(SIGPIPE,SIG_IGN);
	lua_register(L,"stop_program",lua_stop); 
	lua_register(L,"GetSysTick",lua_getsystick); 
	   	
	reg_luasocket(L);
	g_engine = kn_new_engine();
	start(L,argv[1]);
	luaL_loadstring(L,"collectgarbage(collect)");
	if(0 != lua_pcall(L,0,0,0)){
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}
	return 0;
} 