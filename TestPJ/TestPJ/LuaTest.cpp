#include <iostream>
#include "lua.hpp"

// Lua에서 호출할 C++ 함수
int SpawnMonster(lua_State* L)
{
    const char* name = lua_tostring(L, 1);
    int x = (int)lua_tonumber(L, 2);
    int y = (int)lua_tonumber(L, 3);

    std::cout << "C++: SpawnMonster "
        << name << " (" << x << "," << y << ")\n";

    return 0; // 반환값 없음
}

int main()
{
    // 1. Lua 상태 생성
    lua_State* L = luaL_newstate();

    // 2. Lua 기본 라이브러리 로드
    luaL_openlibs(L);

    // 3. C++ 함수를 Lua에 등록
    lua_register(L, "SpawnMonster", SpawnMonster);

    // 4. Lua 스크립트 로드
    if (luaL_dofile(L, "C:/Users/user/Desktop/Academy/TestPJ/lua54/scripts/monster.lua") != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        std::cerr << "Lua load error: " << err << std::endl;
    }

    // 5. Lua 함수 호출

    lua_getglobal(L, "OnMonsterDead"); // 함수 찾기rmfj
    lua_pushnumber(L, 1001);           // 인자 전달

    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
    {
        std::cout << "Lua call error: "
            << lua_tostring(L, -1) << "\n";
    }

    lua_close(L);
    return 0;
}
