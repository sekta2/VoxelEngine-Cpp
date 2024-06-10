#ifndef LOGIC_SCRIPTING_LUA_UTIL_HPP_
#define LOGIC_SCRIPTING_LUA_UTIL_HPP_

#include "LuaState.hpp"

#ifdef __linux__ 
#include <luajit-2.1/luaconf.h>
#include <luajit-2.1/lua.hpp>
#else
#include <lua.hpp>
#endif
#include <glm/glm.hpp>

#include <stdexcept>

namespace lua {
    // function wrappers with number of pushed values as return value

    inline int pushnil(lua_State* L) {
        lua_pushnil(L);
        return 1;
    }

    inline int pushinteger(lua_State* L, lua_Integer x) {
        lua_pushinteger(L, x);
        return 1;
    }

    inline int pushnumber(lua_State* L, lua_Number x) {
        lua_pushnumber(L, x);
        return 1;
    }

    inline int pushivec3(lua_State* L, lua_Integer x, lua_Integer y, lua_Integer z) {
        lua_pushinteger(L, x);
        lua_pushinteger(L, y);
        lua_pushinteger(L, z);
        return 3;
    }

    inline int pushivec3(lua_State* L, glm::ivec3 vec) {
        lua_pushinteger(L, vec.x);
        lua_pushinteger(L, vec.y);
        lua_pushinteger(L, vec.z);
        return 3;
    }

    inline int pushvec3(lua_State* L, glm::vec3 vec) {
        lua_pushnumber(L, vec.x);
        lua_pushnumber(L, vec.y);
        lua_pushnumber(L, vec.z);
        return 3;
    }

    inline int pushvec4(lua_State* L, glm::vec4 vec) {
        lua_pushnumber(L, vec.x);
        lua_pushnumber(L, vec.y);
        lua_pushnumber(L, vec.z);
        lua_pushnumber(L, vec.w);
        return 4;
    }

    inline int pushvec2_arr(lua_State* L, glm::vec2 vec) {
        lua_createtable(L, 2, 0);
        lua_getglobal(L, "vec2_mt");
        lua_setmetatable(L, -2);

        lua_pushnumber(L, vec.x);
        lua_rawseti(L, -2, 1);
        lua_pushnumber(L, vec.y);
        lua_rawseti(L, -2, 2);
        return 1;
    }

    inline int pushvec3_arr(lua_State* L, glm::vec3 vec) {
        lua_createtable(L, 3, 0);
        lua_getglobal(L, "vec3_mt");
        lua_setmetatable(L, -2);

        lua_pushnumber(L, vec.x);
        lua_rawseti(L, -2, 1);
        lua_pushnumber(L, vec.y);
        lua_rawseti(L, -2, 2);
        lua_pushnumber(L, vec.z);
        lua_rawseti(L, -2, 3);
        return 1;
    }

    inline int pushvec4_arr(lua_State* L, glm::vec4 vec) {
        lua_createtable(L, 4, 0);
        lua_getglobal(L, "vec4_mt");
        lua_setmetatable(L, -2);

        lua_pushnumber(L, vec.x);
        lua_rawseti(L, -2, 1);
        lua_pushnumber(L, vec.y);
        lua_rawseti(L, -2, 2);
        lua_pushnumber(L, vec.z);
        lua_rawseti(L, -2, 3);
        lua_pushnumber(L, vec.w);
        lua_rawseti(L, -2, 4);
        return 1;
    }
    inline int pushcolor_arr(lua_State* L, glm::vec4 vec) {
        lua_createtable(L, 4, 0);
        lua_getglobal(L, "color_mt");
        lua_setmetatable(L, -2);

        lua_pushinteger(L, vec.x*255);
        lua_rawseti(L, -2, 1);
        lua_pushinteger(L, vec.y*255);
        lua_rawseti(L, -2, 2);
        lua_pushinteger(L, vec.z*255);
        lua_rawseti(L, -2, 3);
        lua_pushinteger(L, vec.w*255);
        lua_rawseti(L, -2, 4);
        return 1;
    }
    inline int pushcfunction(lua_State* L, lua_CFunction func) {
        lua_pushcfunction(L, func);
        return 1;
    }
    inline int pushstring(lua_State* L, const std::string& str) {
        lua_pushstring(L, str.c_str());
        return 1;
    }

    int pushwstring(lua_State* L, const std::wstring& str);

    inline int pushboolean(lua_State* L, bool value) {
        lua_pushboolean(L, value);
        return 1;
    }
    inline int pushglobals(lua_State* L) {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        return 1;
    }
    inline int pushvalue(lua_State* L, int idx) {
        lua_pushvalue(L, idx);
        return 1;
    }

    inline bool toboolean(lua_State* L, int idx) {
        return lua_toboolean(L, idx);
    }

    inline glm::vec2 tovec2(lua_State* L, int idx) {
        lua_pushvalue(L, idx);
        if (!lua_istable(L, idx) || lua_objlen(L, idx) < 2) {
            throw std::runtime_error("value must be an array of two numbers");
        }
        lua_rawgeti(L, -1, 1);
        lua_Number x = lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 2);
        lua_Number y = lua_tonumber(L, -1); lua_pop(L, 1);
        lua_pop(L, 1);
        return glm::vec2(x, y);
    }

    inline glm::vec4 tocolor(lua_State* L, int idx) {
        lua_pushvalue(L, idx);
        if (!lua_istable(L, -1) || lua_objlen(L, idx) < 4) {
            throw std::runtime_error("RGBA array required");
        }
        lua_rawgeti(L, -1, 1);
        lua_Number r = lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 2);
        lua_Number g = lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 3);
        lua_Number b = lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 4);
        lua_Number a = lua_tonumber(L, -1); lua_pop(L, 1);
        lua_pop(L, 1);
        return glm::vec4(r/255, g/255, b/255, a/255);
    }

    inline const char* require_string(lua_State* L, int idx) {
        if (!lua_isstring(L, idx)) {
            throw luaerror("string expected at "+std::to_string(idx));
        }
        return lua_tostring(L, idx);
    }

    std::wstring require_wstring(lua_State*, int idx);
    int pushvalue(lua_State*, const dynamic::Value& value);
    dynamic::Value tovalue(lua_State*, int idx);

    inline void pop(lua_State* L, int n=1) {
        lua_pop(L, n);
    }

    inline bool getfield(lua_State* L, const std::string& name, int idx=-1) {
        lua_getfield(L, idx, name.c_str());
        if (lua_isnil(L, -1)) {
            lua_pop(L, -1);
            return false;
        }
        return true;
    }

    inline void setfield(lua_State* L, const std::string& name, int idx=-2) {
        lua_setfield(L, idx, name.c_str());
    }

    inline bool getglobal(lua_State* L, const std::string& name) {
        lua_getglobal(L, name.c_str());
        if (lua_isnil(L, lua_gettop(L))) {
            lua_pop(L, lua_gettop(L));
            return false;
        }
        return true;
    }

    inline bool hasglobal(lua_State* L, const std::string& name) {
        lua_getglobal(L, name.c_str());
        if (lua_isnil(L, lua_gettop(L))) {
            lua_pop(L, lua_gettop(L));
            return false;
        }
        lua_pop(L, lua_gettop(L));
        return true;
    }

    inline void setglobal(lua_State* L, const std::string& name) {
        lua_setglobal(L, name.c_str());
    }

    void logError(const std::string& text);
    int call(lua_State*, int argc, int nresults=-1);
    int callNoThrow(lua_State*, int argc);
}

#endif // LOGIC_SCRIPTING_LUA_UTIL_HPP_
