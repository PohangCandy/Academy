#pragma once

//이 부분이 좀 애매하다.
//맵 마다 타일 정보가 다를건데, 맵 인터페이스를 상속받게되는 모든 맵은 필요없는 타입도 무조건 들고가게 된다.
enum ETileType {
	none = 0,//빈칸 nothing
	start,//스타트 지점 start
	end,//끝 지점 end
	obs,//장애물 obstacle
	n,//JPS로 만들어진 노드 nodelist
	v,//JPS로 탐색한 타일 visited
};

struct Grid {
	int x;
	int y;

	bool operator == (Grid a)
	{
		return (this->x == a.x) && (this->y == a.y);
	}
};

class IMap {
public:
	virtual int getwidth() = 0;
	virtual int getheight() = 0;
	virtual bool IsObstacle(int y, int x) = 0;
	virtual ETileType CheckTile(int y, int x) = 0;

	virtual Grid getStart() = 0;
	virtual Grid getGoal() = 0;

	virtual ~IMap() = default;
};