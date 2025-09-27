#pragma once

//이 부분이 좀 애매하다.
//맵 마다 타일 정보가 다를건데, 맵 인터페이스를 상속받게되는 모든 맵은 필요없는 타입도 무조건 들고가게 된다.
enum ETileType {
	none = 0,//빈칸 nothing
	start,//스타트 지점 start
	end,//끝 지점 end
	obs,//장애물 obstacle
	nodelist,//JPS로 만들어진 노드 nodelist
	visited,//JPS로 탐색한 타일 visited
	shortest,//최단거리

	out,//맵을 벗어난 지점
};

enum EDirection
{
	LL, LU, UU, RU, RR, RD, DD, LD
};

struct Grid
{
	int x;
	int y;

	ETileType type;

	float h;
	float g;
	float f;

	Grid* gparent;

	bool operator == (Grid a)
	{
		return (this->x == a.x) && (this->y == a.y);
	}

	EDirection getNodedirection()
	{
		if (x > gparent->x)
		{
			if (y > gparent->y)
			{
				return RD;
			}
			else if (y < gparent->y)
			{
				return RU;
			}
			else
			{
				return RR;
			}
		}
		else if (x < gparent->x)
		{
			if (y > gparent->y)
			{
				return LD;
			}
			else if (y < gparent->y)
			{
				return LU;
			}
			else
			{
				return LL;
			}
		}
		else
		{
			if (y > gparent->y)
			{
				return DD;
			}
			else
			{
				return UU;
			}
		}
	}
};



class IMap {
public:
	virtual int getwidth() = 0;
	virtual int getheight() = 0;
	virtual bool IsObstacle(int y, int x) = 0;
	virtual ETileType CheckTile(int y, int x) = 0;

	virtual Grid* getStart() = 0;
	virtual Grid* getGoal() = 0;
	virtual void ChangeTile(int y, int x, ETileType v) = 0;
	virtual void mapUpdate() = 0;
	virtual void InitMap() = 0;

	virtual float getGridFdata(int y, int x) = 0;
	virtual void setMapData(int x, int y, float g, float h, float F, Grid* parent) = 0;
	virtual Grid* getGrid(int y, int x) = 0;

	virtual ~IMap() = default;
};