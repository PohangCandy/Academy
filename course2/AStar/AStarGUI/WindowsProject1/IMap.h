#pragma once
class IMap {
public:
	virtual bool IsObstacle(int y, int x) = 0;
	virtual int CheckTile(int y, int x) = 0;
	virtual ~IMap() = default;
};