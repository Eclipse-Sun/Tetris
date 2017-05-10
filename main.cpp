/**
 * Tetris 简单交互样例程序
 * https://wiki.botzone.org/index.php?title=Tetris
 * 更新于2017年4月20日：
 * 修正了rotation函数、将交互方式修改为新规则的格式，还有transfer函数里`if (h2 >= MAPHEIGHT)`改为`if (h2 > MAPHEIGHT)`
 */
// 注意：x的范围是1~MAPWIDTH，y的范围是1~MAPHEIGHT
// 数组是先行（y）后列（x）
// 坐标系：原点在左下角

#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <list>
#include <cstring>
using namespace std;

#define MAPWIDTH 10
#define MAPHEIGHT 20

const double Inf_double = 10000000;
#define MAX_DEPTH 1

// 我所在队伍的颜色（0为红，1为蓝，仅表示队伍，不分先后）
int currBotColor;
int enemyColor;

const int elimBonus[] = { 0, 1, 3, 5, 7 };

const int blockShape[7][4][8] = {
	{ { 0,0,1,0,-1,0,-1,-1 },{ 0,0,0,1,0,-1,1,-1 },{ 0,0,-1,0,1,0,1,1 },{ 0,0,0,-1,0,1,-1,1 } },
	{ { 0,0,-1,0,1,0,1,-1 },{ 0,0,0,-1,0,1,1,1 },{ 0,0,1,0,-1,0,-1,1 },{ 0,0,0,1,0,-1,-1,-1 } },
	{ { 0,0,1,0,0,-1,-1,-1 },{ 0,0,0,1,1,0,1,-1 },{ 0,0,-1,0,0,1,1,1 },{ 0,0,0,-1,-1,0,-1,1 } },
	{ { 0,0,-1,0,0,-1,1,-1 },{ 0,0,0,-1,1,0,1,1 },{ 0,0,1,0,0,1,-1,1 },{ 0,0,0,1,-1,0,-1,-1 } },
	{ { 0,0,-1,0,0,1,1,0 },{ 0,0,0,-1,-1,0,0,1 },{ 0,0,1,0,0,-1,-1,0 },{ 0,0,0,1,1,0,0,-1 } },
	{ { 0,0,0,-1,0,1,0,2 },{ 0,0,1,0,-1,0,-2,0 },{ 0,0,0,1,0,-1,0,-2 },{ 0,0,-1,0,1,0,2,0 } },
	{ { 0,0,0,1,-1,0,-1,1 },{ 0,0,-1,0,0,-1,-1,-1 },{ 0,0,0,-1,1,-0,1,-1 },{ 0,0,1,0,0,1,1,1 } }
};// 7种形状(长L| 短L| 反z| 正z| T| 直一| 田格)，4种朝向(上左下右)，8:每相邻的两个分别为x，y


class Tetris{
public:
	const int blockType;   // 标记方块类型的序号 0~6
	int blockX;            // 旋转中心的x轴坐标
	int blockY;            // 旋转中心的y轴坐标
	int orientation;       // 标记方块的朝向 0~3
	const int(*shape)[8]; // 当前类型方块的形状定义
	int color;

	Tetris(int t, int color) : blockType(t), shape(blockShape[t]), color(color)	{ }

	Tetris(const Tetris & tetris_): blockType(tetris_.blockType), blockX(tetris_.blockX), blockY(tetris_.blockY),
									orientation(tetris_.orientation), shape(blockShape[tetris_.blockType]), color(tetris_.color) { }

	inline Tetris &set(int x = -1, int y = -1, int o = -1)
	{
		blockX = x == -1 ? blockX : x;
		blockY = y == -1 ? blockY : y;
		orientation = o == -1 ? orientation : o;
		return *this;
	}
};

struct position{
		int x;
		int y;
		int o;
};

class Decision{
public:
	int x, y, o, t;
	Decision(int x_, int y_, int o_, int t_): x(x_), y(y_), o(o_), t(t_) {}
	inline Decision &set(int x_, int y_, int o_, int t_){
		x = x_; y = y_; o = o_; t = t_;
		return *this;
	}
};

class Node{
	// 复制粘贴的小麻雀
public:
	int transCount_[2], maxHeight_[2];
	int trans_[2][4][MAPWIDTH + 2];
	int elimTotal_[2];
	int cheat_table[MAPHEIGHT + 2][MAPWIDTH + 2][4]; // 计算所有可能落地位置时用的表, 0 : unknown, 1: invalid, 2: valid
	int invalidNextTypeForColor[2][7];
	int gridInfo_[2][MAPHEIGHT + 2][MAPWIDTH + 2]; // 两队的地图
	int typeCountForColor_[2][7]; // 两队的方块数
	int nextTypeForColor_[2]; // 两队的当前方块
	int dead;
	list<Decision> player1Decision, player2Decision; // 所有合法的落地位置

	Node(const int grid[][22][12], const int typeCount[][7], const int * nextType){
		memcpy(gridInfo_, grid, sizeof(gridInfo_));
		memcpy(typeCountForColor_, typeCount, sizeof(typeCountForColor_));
		memcpy(nextTypeForColor_, nextType, sizeof(nextTypeForColor_));
		memset(transCount_, 0, sizeof(transCount_));
		memset(maxHeight_, 0, sizeof(maxHeight_));
		memset(trans_, 0, sizeof(trans_));
		memset(elimTotal_, 0, sizeof(elimTotal_));
		findInvalidNext(); allDecisions();
		dead = -1;
	}

	Node(const Node & currNode, const Decision & player1, const Decision & player2){
		memset(transCount_, 0, sizeof(transCount_));
		memset(maxHeight_, 0, sizeof(maxHeight_));
		memset(trans_, 0, sizeof(trans_));
		memset(elimTotal_, 0, sizeof(elimTotal_));
		memcpy(gridInfo_, currNode.gridInfo_, sizeof(gridInfo_));
		memcpy(nextTypeForColor_, currNode.nextTypeForColor_, sizeof(nextTypeForColor_));
		memcpy(typeCountForColor_, currNode.typeCountForColor_, sizeof(typeCountForColor_));
		Tetris myTetris(nextTypeForColor_[currBotColor], currBotColor);
		myTetris.set(player1.x, player1.y, player1.o);
		place(myTetris);
		Tetris enemyTetris(nextTypeForColor_[enemyColor], enemyColor);
		enemyTetris.set(player2.x, player2.y, player2.o);
		place(enemyTetris);
		nextTypeForColor_[currBotColor] = player2.t;
		nextTypeForColor_[enemyColor] = player1.t;
		++typeCountForColor_[currBotColor][player2.t];
		++typeCountForColor_[enemyColor][player1.t];
		eliminate(0); eliminate(1);
		dead = transfer();
		findInvalidNext(); allDecisions();
	}

	Node(int initType){
		int i;
		for (i = 0; i < MAPHEIGHT + 2; i++)
		{
			gridInfo_[1][i][0] = gridInfo_[1][i][MAPWIDTH + 1] = -2;
			gridInfo_[0][i][0] = gridInfo_[0][i][MAPWIDTH + 1] = -2;
		}
		for (i = 0; i < MAPWIDTH + 2; i++)
		{
			gridInfo_[1][0][i] = gridInfo_[1][MAPHEIGHT + 1][i] = -2;
			gridInfo_[0][0][i] = gridInfo_[0][MAPHEIGHT + 1][i] = -2;
		}
		memset(transCount_, 0, sizeof(transCount_));
		memset(maxHeight_, 0, sizeof(maxHeight_));
		memset(trans_, 0, sizeof(trans_));
		memset(elimTotal_, 0, sizeof(elimTotal_));
		memset(typeCountForColor_, 0, sizeof(typeCountForColor_));
		nextTypeForColor_[0] = nextTypeForColor_[1] = initType;
		++typeCountForColor_[0][initType]; ++typeCountForColor_[1][initType];
		memset(invalidNextTypeForColor, 0, sizeof(invalidNextTypeForColor));
		allDecisions();	dead = -1;
	}

	Node & operator= (const Node & tmp){
		memcpy(transCount_, tmp.transCount_, sizeof(transCount_));
		memcpy(maxHeight_, tmp.maxHeight_, sizeof(maxHeight_));
		memcpy(trans_, tmp.trans_, sizeof(trans_));
		memcpy(elimTotal_, tmp.elimTotal_, sizeof(elimTotal_));
		memcpy(cheat_table, tmp.cheat_table, sizeof(cheat_table)); // 计算所有可能落地位置时用的表, 0 : unknown, 1: invalid, 2: valid
		memcpy(invalidNextTypeForColor, tmp.invalidNextTypeForColor, sizeof(invalidNextTypeForColor));
		memcpy(gridInfo_, tmp.gridInfo_, sizeof(gridInfo_)); // 两队的地图
		memcpy(typeCountForColor_, tmp.typeCountForColor_, sizeof(typeCountForColor_)); // 两队的方块数
		memcpy(nextTypeForColor_, tmp.nextTypeForColor_, sizeof(nextTypeForColor_)); // 两队的当前方块
		dead = tmp.dead;
		player1Decision.clear();
		copy(tmp.player1Decision.begin(), tmp.player1Decision.end(), back_inserter(player1Decision));
		player2Decision.clear();
		copy(tmp.player2Decision.begin(), tmp.player2Decision.end(), back_inserter(player2Decision));
		return *this;
	}


	inline void findInvalidNext(){
		memset(invalidNextTypeForColor, 0, sizeof(invalidNextTypeForColor));
		for(int color = 0; color < 2; ++color){
			int maxCount = -1, minCount = 99;
			for (int i = 0; i < 7; ++i){
				if (typeCountForColor_[color][i] > maxCount)
					maxCount = typeCountForColor_[color][i];
				if (typeCountForColor_[color][i] < minCount)
					minCount = typeCountForColor_[color][i];
			}
			if(maxCount - minCount >= 2){
				for(int t = 0; t < 7; ++t){
					if(typeCountForColor_[color][t] == maxCount)
						invalidNextTypeForColor[color][t] = 1;
				}
			}
		}
	}

	inline bool isValid(const Tetris & examTetris){
		int x = examTetris.blockX, y = examTetris.blockY, o = examTetris.orientation;
		if (o < 0 || o > 3)
			return false;
		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++){
			//cout << "haha!\n";
			tmpX = x + examTetris.shape[o][2 * i];
			tmpY = y + examTetris.shape[o][2 * i + 1];
			if (tmpX < 1 || tmpX > MAPWIDTH ||
				tmpY < 1 || tmpY > MAPHEIGHT ||
				gridInfo_[examTetris.color][tmpY][tmpX] != 0)
				return false;
		}
		return true;
	}

	inline bool onGround(const Tetris & examTetris){
		Tetris nextTetris(examTetris);
		--nextTetris.blockY;
		if (isValid(examTetris) && !isValid(nextTetris))
			return true;
		return false;
	}

	// 将方块放置在场地上
	inline bool place(const Tetris & examTetris){
		if (!onGround(examTetris))
			return false;

		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++){
			tmpX = examTetris.blockX + examTetris.shape[examTetris.orientation][2 * i];
			tmpY = examTetris.blockY + examTetris.shape[examTetris.orientation][2 * i + 1];
			gridInfo_[examTetris.color][tmpY][tmpX] = 2;
		}
		return true;
	}

	inline double eval(){
		return elimTotal_[currBotColor];
	}

	// 检查能否从场地顶端直接落到当前位置
	inline bool checkDirectDropTo(int color, int blockType, int x, int y, int o){
		const int *def = blockShape[blockType][o];
		for (; y <= MAPHEIGHT; y++)
			for (int i = 0; i < 4; i++)
			{
				int _x = def[i * 2] + x, _y = def[i * 2 + 1] + y;
				if (_y > MAPHEIGHT)
					continue;
				if (_y < 1 || _x < 1 || _x > MAPWIDTH || gridInfo_[color][_y][_x])
					return false;
			}
		return true;
	}

	// 消去行
	void eliminate(int color){
		int &count = transCount_[color] = 0;
		int i, j, emptyFlag, fullFlag;
		maxHeight_[color] = MAPHEIGHT;
		for (i = 1; i <= MAPHEIGHT; i++)
		{
			emptyFlag = 1;
			fullFlag = 1;
			for (j = 1; j <= MAPWIDTH; j++)
			{
				if (gridInfo_[color][i][j] == 0)
					fullFlag = 0;
				else
					emptyFlag = 0;
			}
			if (fullFlag)
			{
				for (j = 1; j <= MAPWIDTH; j++)
				{
					// 注意这里只转移以前的块，不包括最后一次落下的块（“撤销最后一步”）
					trans_[color][count][j] = gridInfo_[color][i][j] == 1 ? 1 : 0;
					gridInfo_[color][i][j] = 0;
				}
				count++;
			}
			else if (emptyFlag)
			{
				maxHeight_[color] = i - 1;
				break;
			}
			else
				for (j = 1; j <= MAPWIDTH; j++)
				{
					gridInfo_[color][i - count][j] =
						gridInfo_[color][i][j] > 0 ? 1 : gridInfo_[color][i][j];
					if (count)
						gridInfo_[color][i][j] = 0;
				}
		}
		maxHeight_[color] -= count;
		elimTotal_[color] += elimBonus[count];
	}

	// 转移双方消去的行，返回-1表示继续，否则返回输者
	int transfer(){
		int color1 = 0, color2 = 1;
		if (transCount_[color1] == 0 && transCount_[color2] == 0)
			return -1;
		if (transCount_[color1] == 0 || transCount_[color2] == 0)
		{
			if (transCount_[color1] == 0 && transCount_[color2] > 0)
				swap(color1, color2);
			int h2;
			maxHeight_[color2] = h2 = maxHeight_[color2] + transCount_[color1];
			if (h2 > MAPHEIGHT)
				return color2;
			int i, j;

			for (i = h2; i > transCount_[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo_[color2][i][j] = gridInfo_[color2][i - transCount_[color1]][j];

			for (i = transCount_[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo_[color2][i][j] = trans_[color1][i - 1][j];
			return -1;
		}
		else
		{
			int h1, h2;
			maxHeight_[color1] = h1 = maxHeight_[color1] + transCount_[color2];//从color1处移动count1去color2
			maxHeight_[color2] = h2 = maxHeight_[color2] + transCount_[color1];

			if (h1 > MAPHEIGHT) return color1;
			if (h2 > MAPHEIGHT) return color2;

			int i, j;
			for (i = h2; i > transCount_[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo_[color2][i][j] = gridInfo_[color2][i - transCount_[color1]][j];

			for (i = transCount_[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo_[color2][i][j] = trans_[color1][i - 1][j];

			for (i = h1; i > transCount_[color2]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo_[color1][i][j] = gridInfo_[color1][i - transCount_[color2]][j];

			for (i = transCount_[color2]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo_[color1][i][j] = trans_[color2][i - 1][j];

			return -1;
		}
	}

	// 打印场地用于调试
	inline void printField(){
#ifndef _BOTZONE_ONLINE
		static const char *i2s[] = {
			"~~",
			"~~",
			"  ",
			"[]",
			"##"
		};
		cout << "~~：墙，[]：块，##：新块" << endl;
		for (int y = MAPHEIGHT + 1; y >= 0; y--){
			for (int x = 0; x <= MAPWIDTH + 1; x++)
				cout << i2s[gridInfo_[0][y][x] + 2];
			for (int x = 0; x <= MAPWIDTH + 1; x++)
				cout << i2s[gridInfo_[1][y][x] + 2];
			cout << endl;
		}
#endif
	}

	inline bool color_cheat_table(int color, int blockType, int x, int y, int o){
		if(cheat_table[y][x][o])
			return cheat_table[y][x][o] - 1;
		Tetris tmp_tetris(blockType, color);
		tmp_tetris.set(x, y, o);
		if(!isValid(tmp_tetris)){
			cheat_table[y][x][o] = 1;
			return false;
		}
		if(checkDirectDropTo(color, blockType, x, y, o)){
			cheat_table[y][x][o] = 2;
			return true;
		}
		if(color_cheat_table(color, blockType, x, y, (o + 1) % 4) || color_cheat_table(color, blockType, x - 1, y, o)
			|| color_cheat_table(color, blockType, x + 1, y, o) || color_cheat_table(color, blockType, x, y + 1, o)){
			cheat_table[y][x][o] = 2;
			return true;
		}
		cheat_table[y][x][o] = 1;
		return false;
	}

	// 求所有的决策
	inline void allDecisions(){
		// 初始化
		memset(cheat_table, 0, sizeof(cheat_table));
		player1Decision.clear();
		int color = currBotColor, blockType = nextTypeForColor_[color];
			// 循环求出所有合法的落地点
		Tetris tmp_tetris(blockType, color);
		for(int y = 1; y <= MAPHEIGHT; ++y){
			for(int x = 1; x <= MAPWIDTH; ++x){
				for(int o = 0; o < 4; ++o){
					tmp_tetris.set(x, y, o);
					if(onGround(tmp_tetris)){
						if(color_cheat_table(color, blockType, x, y, o)){
							for(int t = 0; t < 7; ++t){
								if(!invalidNextTypeForColor[1 - color][t]){
									Decision d(x, y, o, t);
									player1Decision.push_back(d);
								}
							}
						}
					}
				}
			}
		}

		memset(cheat_table, 0, sizeof(cheat_table));
		player2Decision.clear();
		color = enemyColor; blockType = nextTypeForColor_[color];

		// 循环求出所有合法的落地点
		Tetris tmp__tetris(blockType, color);
		for(int y = 1; y <= MAPHEIGHT; ++y){
			for(int x = 1; x <= MAPWIDTH; ++x){
				for(int o = 0; o < 4; ++o){
					tmp__tetris.set(x, y, o);
					if(onGround(tmp__tetris)){
						if(color_cheat_table(color, blockType, x, y, o)){
							for(int t = 0; t < 7; ++t){
								if(!invalidNextTypeForColor[1 - color][t]){
									Decision d(x, y, o, t);
									player2Decision.push_back(d);
								}
							}
						}
					}
				}
			}
		}
	}
};


class SingleNode{
public:
	int elimRows;
    bool dead;//有没有死
    bool position_step;// ＝ 1，下一步该找位置；= 0, 下一步该给方块
    int gridInfo_[MAPHEIGHT + 2][MAPWIDTH + 2];//这个队伍的地图
    int typeCountForColor_[7];//方块数
    int nextTypeForColor_; //当前方块， position step = 1 时它有意义
    list<position> availablePosition; //position step = 1 时它有意义
    list<int> availableTetris; //position step = 0 时它有意义
    Tetris lastTetris; //上一次放的方块
    int cheat_table[MAPHEIGHT + 2][MAPWIDTH + 2][4];

	SingleNode(const Node & currNode, bool isMine):lastTetris(-1, -1){
		elimRows = 0;
		int color = isMine ? currBotColor : enemyColor;
		dead = false; position_step = true;
		memcpy(gridInfo_, currNode.gridInfo_[color], sizeof(gridInfo_));
		memcpy(typeCountForColor_, currNode.typeCountForColor_[color], sizeof(typeCountForColor_));
		nextTypeForColor_ = currNode.nextTypeForColor_[color];
		//lastTetris(-1, -1);
		availablePosition.clear();
		if(isMine){
			for(list<Decision>::const_iterator itr = currNode.player1Decision.begin(); itr != currNode.player1Decision.end(); ++itr){
				position p = {itr->x, itr->y, itr->o};
				availablePosition.push_back(p);
			}
		}
		else{
			for(list<Decision>::const_iterator itr = currNode.player2Decision.begin(); itr != currNode.player2Decision.end(); ++itr){
				position p = {itr->x, itr->y, itr->o};
				availablePosition.push_back(p);
			}

		}
		availableTetris.clear();
		for(int i = 0; i < 7; ++i){
			if(!currNode.invalidNextTypeForColor[color][i])
				availableTetris.push_back(i);
		}
	}

    SingleNode(const SingleNode & currSingleNode, const position & place_position) : lastTetris(currSingleNode.nextTypeForColor_, -1){
		lastTetris.set(place_position.x, place_position.y, place_position.o);
		dead = currSingleNode.dead;
		elimRows = currSingleNode.elimRows;
		position_step = false;
		memcpy(gridInfo_, currSingleNode.gridInfo_, sizeof(gridInfo_));
		memcpy(typeCountForColor_, currSingleNode.typeCountForColor_, sizeof(typeCountForColor_));
		nextTypeForColor_ = -1;
		availablePosition.clear();
		availableTetris.clear();
		copy(currSingleNode.availableTetris.begin(), currSingleNode.availableTetris.end(), back_inserter(availableTetris));
		place(lastTetris); eliminate();
		printField();
    }// 从状态 1 给出 position 向状态 0 转化
    SingleNode(const SingleNode & currSingleNode, const int give_colortype) : lastTetris(currSingleNode.lastTetris) {
    	elimRows = currSingleNode.elimRows;
    	position_step = true;
    	memcpy(gridInfo_, currSingleNode.gridInfo_, sizeof(gridInfo_));
    	memcpy(typeCountForColor_, currSingleNode.typeCountForColor_, sizeof(typeCountForColor_));
    	++typeCountForColor_[give_colortype];
    	nextTypeForColor_ = give_colortype;
    	memset(cheat_table, 0, sizeof(cheat_table));
		availablePosition.clear();
		Tetris tmp_tetris(give_colortype, -1);
		for(int y = 1; y <= MAPHEIGHT; ++y){
			for(int x = 1; x <= MAPWIDTH; ++x){
				for(int o = 0; o < 4; ++o){
					tmp_tetris.set(x, y, o);
					if(onGround(tmp_tetris)){
						if(color_cheat_table(give_colortype, x, y, o)){
							position p = {x, y, o};
							availablePosition.push_back(p);
							}
						}
					}
				}
			}
		dead = availablePosition.empty();
		findInvalidNext();
    }

	// 从状态 0 给出 colortype 向状态 1 转化

	inline bool isValid(const Tetris & examTetris){
		int x = examTetris.blockX, y = examTetris.blockY, o = examTetris.orientation;
		if (o < 0 || o > 3)
			return false;
		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++){
			//cout << "haha!\n";
			tmpX = x + examTetris.shape[o][2 * i];
			tmpY = y + examTetris.shape[o][2 * i + 1];
			if (tmpX < 1 || tmpX > MAPWIDTH ||
				tmpY < 1 || tmpY > MAPHEIGHT ||
				gridInfo_[tmpY][tmpX] != 0)
				return false;
		}
		return true;
	}

	inline void printField(){
#ifndef _BOTZONE_ONLINE
		static const char *i2s[] = {
			"~~",
			"~~",
			"  ",
			"[]",
			"##"
		};
		cout << "~~：墙，[]：块，##：新块" << endl;
		for (int y = MAPHEIGHT + 1; y >= 0; y--){
			for (int x = 0; x <= MAPWIDTH + 1; x++)
				cout << i2s[gridInfo_[y][x] + 2];
			cout << endl;
		}
#endif
	}

	inline bool onGround(const Tetris & examTetris){
		Tetris nextTetris(examTetris);
		--nextTetris.blockY;
		if (isValid(examTetris) && !isValid(nextTetris))
			return true;
		return false;
	}

	inline bool checkDirectDropTo(int blockType, int x, int y, int o){
		const int *def = blockShape[blockType][o];
		for (; y <= MAPHEIGHT; y++)
			for (int i = 0; i < 4; i++)
			{
				int _x = def[i * 2] + x, _y = def[i * 2 + 1] + y;
				if (_y > MAPHEIGHT)
					continue;
				if (_y < 1 || _x < 1 || _x > MAPWIDTH || gridInfo_[_y][_x])
					return false;
			}
		return true;
	}

	inline bool place(const Tetris & examTetris){
		if (!onGround(examTetris))
			return false;

		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++){
			tmpX = examTetris.blockX + examTetris.shape[examTetris.orientation][2 * i];
			tmpY = examTetris.blockY + examTetris.shape[examTetris.orientation][2 * i + 1];
			gridInfo_[tmpY][tmpX] = 2;
		}
		return true;
	}
	inline void eliminate(){
		int count = 0;
		int i, j, emptyFlag, fullFlag;
		int maxHeight_ = MAPHEIGHT;
		//int trans_[MAPHEIGHT + 2][MAPWIDTH + 2] = {0};
		for (i = 1; i <= MAPHEIGHT; i++)
		{
			emptyFlag = 1;
			fullFlag = 1;
			for (j = 1; j <= MAPWIDTH; j++)
			{
				if (gridInfo_[i][j] == 0)
					fullFlag = 0;
				else
					emptyFlag = 0;
			}
			if (fullFlag)
			{
				for (j = 1; j <= MAPWIDTH; j++)
				{
					// 注意这里只转移以前的块，不包括最后一次落下的块（“撤销最后一步”）
					//trans_[count][j] = gridInfo_[i][j] == 1 ? 1 : 0;
					gridInfo_[i][j] = 0;
				}
				count++;
			}
			else if (emptyFlag)
			{
				maxHeight_ = i - 1;
				break;
			}
			else
				for (j = 1; j <= MAPWIDTH; j++)
				{
					gridInfo_[i - count][j] =
						gridInfo_[i][j] > 0 ? 1 : gridInfo_[i][j];
					if (count)
						gridInfo_[i][j] = 0;
				}
		}
		maxHeight_ -= count;
		elimRows += count;
	}

	inline bool color_cheat_table(int blockType, int x, int y, int o){
		/*cin.get();
		for(int y = 0; y < MAPHEIGHT + 2; ++y){
			for(int x = 0; x < MAPWIDTH + 2; ++x){
				for(int o = 0; o < 4; ++o){
					cout << cheat_table[y][x][o] << ' ';
				}
				cout << "    ";
			}
			cout << endl;
		}*/
		if(cheat_table[y][x][o])
			return cheat_table[y][x][o] - 1;
		Tetris tmp_tetris(blockType, -1);
		tmp_tetris.set(x, y, o);
		if(!isValid(tmp_tetris)){
			cheat_table[y][x][o] = 1;
			return false;
		}
		if(checkDirectDropTo(blockType, x, y, o)){
			cheat_table[y][x][o] = 2;
			return true;
		}
		if(color_cheat_table(blockType, x, y, (o + 1) % 4) || color_cheat_table(blockType, x - 1, y, o)
			|| color_cheat_table(blockType, x + 1, y, o) || color_cheat_table(blockType, x, y + 1, o)){
			cheat_table[y][x][o] = 2;
			return true;
		}
		cheat_table[y][x][o] = 1;
		return false;
	}

	inline void findInvalidNext(){
		availableTetris.clear();
		int maxCount = 0, minCount = 99;
		for (int i = 0; i < 7; ++i){
			if (typeCountForColor_[i] > maxCount)
				maxCount = typeCountForColor_[i];
			if (typeCountForColor_[i] < minCount)
				minCount = typeCountForColor_[i];
		}
		if(maxCount - minCount >= 2){
			for(int t = 0; t < 7; ++t){
				if(typeCountForColor_[t] != maxCount)
                    availableTetris.push_back(t);
			}
		}
		else
			for(int t = 0; t < 7; ++t)
				availableTetris.push_back(t);
	}

	    inline double eval(){
        int LowestHeight[4][7] = {{-1, -1, -1, -1, 0, -1, 0},{-1, -1, -1, -1, -1, 0, -1},
            {0, 0, 0, 0, -1, -2, -1}, {-1, -1, -1, -1, -1, 0, 0}};
        int HighstHeight[4][7] = {{0, 0, 0, 0, 1, 2, 1}, {1, 1, 1, 1, 1, 0, 0},
            {1, 1, 1, 1, 0, 1, 0}, {1, 1, 1, 1, 1, 0, 1}};
        double AverageHeight[4][7] = {{-0.25, -0.25, -0.5, -0.5, 0.25, 0.5, 0.5},
            {-0.25, 0.25, 0, 0, 0, 0, -0.5},
            {0.25, 0.25, 0.5, 0.5, -0.25, -0.5, -0.5},
            {0.25, -0.25, 0, 0, 0, 0, 0.5}};
        int f1LowestHeightOfPosition = lastTetris.blockY + LowestHeight[lastTetris.orientation][lastTetris.blockType];
        int f2HighestHeightOfPosition = lastTetris.blockY + HighstHeight[lastTetris.orientation][lastTetris.blockType];
        double f3averageHeightOfPosition = lastTetris.blockY + AverageHeight[lastTetris.orientation][lastTetris.blockType];
        int height_[MAPWIDTH + 2];
        for(int col = 1; col <= MAPWIDTH; ++col){
            height_[col] = 0;
            for(int i = MAPHEIGHT; i >= 1; --i){
                if(gridInfo_[i][col] != 0){
                    height_[col] = i;
                    break;
                }
            }
        }
        int f4NumberofHoles = 0;
        for(int col = 1; col <= MAPWIDTH; ++col){
            for(int i = height_[col]; i >= 1; --i){
                    if(gridInfo_[i][col] == 0)
                        ++f4NumberofHoles;
            }
        }
        int f5NumberofLinesCompleted = elimRows;
        int f6SurfaceRoughness = 0;
        for(int col = 2; col <= MAPWIDTH - 2; ++col)
            f6SurfaceRoughness += abs(height_[col] - height_[col + 1]);
        f6SurfaceRoughness += height_[2] - height_[1] + height_[MAPWIDTH - 1] - height_[MAPWIDTH];
        int f7SquaresAboveHoles = 0;
        for(int col = 1; col < MAPWIDTH; ++col){
            for(int i = height_[col]; i >= 1; --i){
                if(gridInfo_[i][col] == 0){
                    f7SquaresAboveHoles += height_[col] - i + 1;
                    break;
                }
            }
        }
        int f8SpacesAjacenttoHoles = 0;
        for(int col = 1; col <= MAPWIDTH; ++col){
            for(int i = height_[col]; i >= 1; --i){
                if(gridInfo_[i][col] == 0){
                    if(col > 1 && height_[col - 1] < i)
                        ++f8SpacesAjacenttoHoles;
                    if(col < MAPWIDTH && height_[col + 1] < i)
                        ++f8SpacesAjacenttoHoles;
                }
            }
        }
        return -40. * f1LowestHeightOfPosition - 20. * f2HighestHeightOfPosition - 10. * f3averageHeightOfPosition - 300. * f4NumberofHoles + 490. * f5NumberofLinesCompleted - 70. * f6SurfaceRoughness - 50. * f7SquaresAboveHoles + 20. * f8SpacesAjacenttoHoles;
    }
};

double alphabeta_singlemap(SingleNode &currSingleNode_, int depth, double alpha, double beta, bool is_me, int init_depth, position* BestPosition){
   if(depth == 0 || currSingleNode_.dead == 1)
       return currSingleNode_.eval();
    if(is_me == 1){
        double val = -Inf_double;
        for(list<position>::iterator itr = currSingleNode_.availablePosition.begin(); itr != currSingleNode_.availablePosition.end(); ++itr){
            SingleNode thisChild(currSingleNode_, *itr);
            double tmp = alphabeta_singlemap(thisChild, depth - 1, alpha, beta, 0, init_depth, BestPosition);
            if(tmp > val){
                val = tmp;
                if(depth == init_depth){
                    BestPosition->x = itr->x;
                    BestPosition->y = itr->y;
                    BestPosition->o = itr->o;
                }
            }
            if(tmp > alpha)
                alpha = tmp;
            if(beta <= alpha)
                break;
        }
        return val;
    }
    else{
        double val = Inf_double;
        for(list<int>::iterator itr = currSingleNode_.availableTetris.begin(); itr != currSingleNode_.availableTetris.end(); ++itr){
            SingleNode thisChild(currSingleNode_, *itr);
            double tmp = alphabeta_singlemap(thisChild, depth - 1, alpha, beta, 1, init_depth, BestPosition);
            if(tmp < val){
                val = tmp;
                //if(depth == init_depth)
                   // *WorthTetris = *itr;
            }
            if(tmp < alpha)
                alpha = tmp;
            if(beta <= alpha)
                break;
        }
        return val;
    }
}

void makeMyDecision(const Node &currNode_, int init_depth1, int init_depth2){
    SingleNode MycurrSingleNode(currNode_, 1);
    SingleNode HiscurrSingleNode(currNode_, 0);
    position MyBestPosition = MycurrSingleNode.availablePosition.front();
    int HisWorstTeris = HiscurrSingleNode.availableTetris.front();
    alphabeta_singlemap(MycurrSingleNode, init_depth1, -Inf_double, Inf_double, 1, init_depth1, &MyBestPosition);
 /*   double minmax_ = Inf_double;
    for(list<int>::iterator itr = HiscurrSingleNode.availableTetris.begin(); itr != HiscurrSingleNode.availableTetris.end(); ++itr){
        SingleNode H1(HiscurrSingleNode, *itr);
        double max_ = -Inf_double;
        for(list<position>::iterator itr2 = HiscurrSingleNode.availablePosition.begin(); itr2 != HiscurrSingleNode.availablePosition.end(); ++itr2){
            SingleNode H2(H1, *itr2);
            position HisBestPosition = H2.availablePosition.front();
            double tmp = alphabeta_singlemap(H2, init_depth2, -Inf_double, Inf_double, 1, init_depth2, &HisBestPosition);
            if(tmp > max_)
                max_ = tmp;
        }
        if (max_ < minmax_){
            minmax_ = max_;
            HisWorstTeris = *itr;
        }
    }*/
    cout << HisWorstTeris << " " << MyBestPosition.x << " " << MyBestPosition.y << " " << MyBestPosition.o;
}
int main()
{
	// 加速输入
	istream::sync_with_stdio(false);
	//srand(time(NULL));
	//init();

	int turnID, blockType;
	cin >> turnID;

	// 先读入第一回合，得到自己的颜色
	// 双方的第一块肯定是一样的
	cin >> blockType >> currBotColor;
	enemyColor = 1 - currBotColor;
	Node currNode(blockType);
	// 平台保证所有输入都是合法输入
	for (int i = 1; i < turnID; i++){
		int t1, x1, y1, o1, t2, x2, y2, o2;
		// 根据这些输入输出逐渐恢复状态到当前回合
		cin >> t1 >> x1 >> y1 >> o1 >> t2 >> x2 >> y2 >> o2;
		Decision player1(x1, y1, o1, t1), player2(x2, y2, o2, t2);
		Node newNode(currNode, player1, player2);
		currNode = newNode;
	}

	//int blockForEnemy, finalX, finalY, finalO;

	// 做出决策（你只需修改以下部分）

	// 遇事不决先输出（平台上编译不会输出）
	currNode.printField();

	/*finalX = currNode.player1Decision.front().x;
	finalY = currNode.player1Decision.front().y;
	finalO = currNode.player1Decision.front().o;
	blockForEnemy = currNode.player1Decision.front().t;

	// 决策结束，输出结果（你只需修改以上部分）


	cout << blockForEnemy << " " << finalX << " " << finalY << " " << finalO;*/

	makeMyDecision(currNode, 0, 0);

	cin.get();cin.get();

	return 0;
}
