
#include <algorithm>
#include "lua_compat.h"
#include "i_system.h"
#include "str_util.h"
#include "w_files.h"
#include "con_var.h"
#include "../lua_debugger.h"

// Enable Lua debugging
#ifdef EDGE_LUA_DEBUG
static bool lua_debug = true;
#else
static bool lua_debug = false;
#endif

static void LUA_GetRequirePackPath(const char *name, std::string &out)
{
    std::string require_name(name);
    std::replace(require_name.begin(), require_name.end(), '.', '/');
    out = epi::STR_Format("scripts/lua/%s.lua", require_name.c_str());
}

static int LUA_PackLoader(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    std::string pack_name;
    LUA_GetRequirePackPath(name, pack_name);

    epi::file_c *file = W_OpenPackFile(pack_name);

    if (!file)
    {
        I_Error("LUA: %s.lua: NOT FOUND\n", name);
        return 0;
    }

    // TODO: Fix me ReadText is copying string on return, and should be taking a reference
    std::string source = file->ReadText();

    delete file;

    int result = luaL_dostring(L, source.c_str());
    if (result != LUA_OK)
    {
        I_Error("LUA: %s.lua: %s\n", name, lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }

    return 1;
}

static int LUA_PackSearcher(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    // NOTE: This is the final searcher, so assume the loader will find the file
    // If not, it will error with a not found.
    // If need additional searchers, will need to revisit this
    /*
    std::string pack_name;
    LUA_GetRequirePackPath(name, pack_name);

    // TODO: This doesn't work with qualified paths, so uses W_OpenPackFile below
    //if (W_CheckPackForName(pack_name) == -1)
    //{
    //    return 0;
    //}

    epi::file_c *file = W_OpenPackFile(pack_name);

    if (!file)
    {
        I_Error("LUA: %s.lua: NOT FOUND\n", name);
        return 0;
    }

    delete file;
    */

    lua_pushcfunction(L, LUA_PackLoader);
    lua_pushstring(L, name);
    return 2;
}

void LUA_DoString(lua_State *L, const char *source)
{
    int ret = luaL_dostring(L, source);

    if (ret != 0)
    {
        I_Error("LUA: %s\n", lua_tostring(L, -1));
    }
}

void LUA_DoFile(lua_State *L, const std::string &name)
{
    epi::file_c *file = W_OpenPackFile(name);

    if (file)
    {
        std::string source = file->ReadText();
        int         top    = lua_gettop(L);
        int         ret    = luaL_dostring(L, source.c_str());
        if (ret != 0)
        {
            I_Error("LUA: %s\n", lua_tostring(L, -1));
        }

        lua_settop(L, top);
        delete file;
    }
}

// NOP dbg() for when debugger is disabled and someone has left some breakpoints in code
static bool dbg_nop_warn = false;
static int  LUA_DbgNOP(lua_State *L)
{
    if (!dbg_nop_warn)
    {
        dbg_nop_warn = true;
        I_Warning("LUA: dbg() called without lua_debug being set.  Please check that a stray dbg call didn't get left "
                  "in source.");
    }
    return 0;
}

void LUA_CallGlobalFunction(lua_State *L, const char *function_name)
{
    lua_getglobal(L, function_name);
    int status = 0;
    if (lua_debug)
    {
        dbg_pcall(L, 0, 0, 0);
    }
    else
    {
        lua_pcall(L, 0, 0, 0);
    }

    if (status != LUA_OK)
    {
        I_Error("LUA: %s\n", lua_tostring(L, -1));
    }
}

lua_State *LUA_CreateVM()
{
    // we could specify a lua allocator, which would be a good idea to hook up to a debug allocator
    // library for tracing l = lua_newstate(lua_Alloc alloc, nullptr);

    lua_State *L = luaL_newstate();

    /*
    ** these libs are loaded by lua.c and are readily available to any Lua
    ** program
    */
    const luaL_Reg loadedlibs[] = {{LUA_GNAME, luaopen_base},          {LUA_LOADLIBNAME, luaopen_package},
                                   {LUA_COLIBNAME, luaopen_coroutine}, {LUA_TABLIBNAME, luaopen_table},
                                   {LUA_STRLIBNAME, luaopen_string},   {LUA_MATHLIBNAME, luaopen_math},
                                   {LUA_UTF8LIBNAME, luaopen_utf8},    {NULL, NULL}};

    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++)
    {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1); /* remove lib */
    }

    // replace searchers with only preload and custom searcher
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");
    lua_newtable(L);
    lua_geti(L, -2, 1);
    lua_seti(L, -2, 1);
    lua_pushcfunction(L, LUA_PackSearcher);
    lua_seti(L, -2, 2);
    lua_setfield(L, -3, "searchers");
    // pop searchers off stack
    lua_pop(L, 1);

    // clear out search path and loadlib
    lua_pushnil(L);
    lua_setfield(L, -2, "loadlib");
    lua_pushnil(L);
    lua_setfield(L, -2, "searchpath");
    // pop package off stack
    lua_pop(L, 1);

    if (lua_debug)
    {
        luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1);
        luaL_requiref(L, LUA_IOLIBNAME, luaopen_io, 1);
        luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, 1);
        lua_pop(L, 3);
        dbg_setup_default(L);
    }
    else
    {
        lua_pushcfunction(L, LUA_DbgNOP);
        lua_setglobal(L, "dbg");
    }

    SYS_ASSERT(!lua_gettop(L));

    return L;
}