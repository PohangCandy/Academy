function OnMonsterDead(monsterId)
    print("Lua: Monster dead with id", monsterId)
    SpawnMonster("Zombie", 10, 20)
end