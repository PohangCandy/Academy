#pragma once
class IMap {
public:
	virtual int getwidth() = 0;
	virtual int getheight() = 0;
	virtual bool IsObstacle(int y, int x) = 0;
	virtual int CheckTile(int y, int x) = 0;
	virtual ~IMap() = default;
};