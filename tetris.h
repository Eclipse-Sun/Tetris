/**
 * 基于 Tetris 简单交互样例程序 (更新于2017年4月20日)
 * https://wiki.botzone.org/index.php?title=Tetris
 **/
// 注意：x的范围是1~MAPWIDTH，y的范围是1~MAPHEIGHT
// 数组是先行（y）后列（x）
// 坐标系：原点在左下角

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <random>
#include <string>

using namespace std;

#define MAPWIDTH 10
#define MAPHEIGHT 20

// int what;

const double Inf_double = 10000000;

// 我所在队伍的颜色（0为红，1为蓝，仅表示队伍，不分先后）
int currBotColor;
int enemyColor;

const int elimBonus[] = {0, 1, 3, 5, 7};

const int blockShape[7][4][8] = {{{0, 0, 1, 0, -1, 0, -1, -1},
                                  {0, 0, 0, 1, 0, -1, 1, -1},
                                  {0, 0, -1, 0, 1, 0, 1, 1},
                                  {0, 0, 0, -1, 0, 1, -1, 1}},
                                 {{0, 0, -1, 0, 1, 0, 1, -1},
                                  {0, 0, 0, -1, 0, 1, 1, 1},
                                  {0, 0, 1, 0, -1, 0, -1, 1},
                                  {0, 0, 0, 1, 0, -1, -1, -1}},
                                 {{0, 0, 1, 0, 0, -1, -1, -1},
                                  {0, 0, 0, 1, 1, 0, 1, -1},
                                  {0, 0, -1, 0, 0, 1, 1, 1},
                                  {0, 0, 0, -1, -1, 0, -1, 1}},
                                 {{0, 0, -1, 0, 0, -1, 1, -1},
                                  {0, 0, 0, -1, 1, 0, 1, 1},
                                  {0, 0, 1, 0, 0, 1, -1, 1},
                                  {0, 0, 0, 1, -1, 0, -1, -1}},
                                 {{0, 0, -1, 0, 0, 1, 1, 0},
                                  {0, 0, 0, -1, -1, 0, 0, 1},
                                  {0, 0, 1, 0, 0, -1, -1, 0},
                                  {0, 0, 0, 1, 1, 0, 0, -1}},
                                 {{0, 0, 0, -1, 0, 1, 0, 2},
                                  {0, 0, 1, 0, -1, 0, -2, 0},
                                  {0, 0, 0, 1, 0, -1, 0, -2},
                                  {0, 0, -1, 0, 1, 0, 2, 0}},
                                 {{0, 0, 0, 1, -1, 0, -1, 1},
                                  {0, 0, -1, 0, 0, -1, -1, -1},
                                  {0, 0, 0, -1, 1, -0, 1, -1},
                                  {0, 0, 1, 0, 0, 1, 1, 1}}};

const int LowestHeight[4][7] = {{-1, -1, -1, -1, 0, -1, 0},
                                {-1, -1, -1, -1, -1, 0, -1},
                                {0, 0, 0, 0, -1, -2, -1},
                                {-1, -1, -1, -1, -1, 0, 0}};

const int HighstHeight[4][7] = {{0, 0, 0, 0, 1, 2, 1},
                                {1, 1, 1, 1, 1, 0, 0},
                                {1, 1, 1, 1, 0, 1, 0},
                                {1, 1, 1, 1, 1, 0, 1}};

const double AverageHeight[4][7] = {{-0.25, -0.25, -0.5, -0.5, 0.25, 0.5, 0.5},
                                    {-0.25, 0.25, 0, 0, 0, 0, -0.5},
                                    {0.25, 0.25, 0.5, 0.5, -0.25, -0.5, -0.5},
                                    {0.25, -0.25, 0, 0, 0, 0, 0.5}};

class Tetris {
public:
  const int blockType;   // 标记方块类型的序号 0~6
  int blockX;            // 旋转中心的x轴坐标
  int blockY;            // 旋转中心的y轴坐标
  int orientation;       // 标记方块的朝向 0~3
  const int (*shape)[8]; // 当前类型方块的形状定义
  int color;

  Tetris(int t, int color) : blockType(t), shape(blockShape[t]), color(color) {}

  Tetris(const Tetris &tetris_)
      : blockType(tetris_.blockType), blockX(tetris_.blockX),
        blockY(tetris_.blockY), orientation(tetris_.orientation),
        shape(blockShape[tetris_.blockType]), color(tetris_.color) {}

  inline Tetris &set(int x = -1, int y = -1, int o = -1) {
    blockX = x == -1 ? blockX : x;
    blockY = y == -1 ? blockY : y;
    orientation = o == -1 ? orientation : o;
    return *this;
  }
};

struct position {
  int x;
  int y;
  int o;
};

class parameter {
public:
  double x[7];
  parameter(double x1_, double x2_, double x4_, double x5_, double x6_,
            double x9_, double x10_) {
    x[0] = x1_;
    x[1] = x2_;
    x[2] = x4_;
    x[3] = x5_;
    x[4] = x6_;
    x[5] = x9_;
    x[6] = x10_;
  }
  parameter(double x_[7]) {
    for (int i = 0; i < 7; ++i)
      x[i] = x_[i];
  }
  parameter(const parameter &p) {
    for (int i = 0; i < 7; ++i)
      x[i] = p.x[i];
  }
  inline parameter &operator=(const parameter &p) {
    for (int i = 0; i < 7; ++i)
      x[i] = p.x[i];
  }
  inline parameter &operator+=(const parameter &p) {
    for (int i = 0; i < 7; ++i)
      x[i] += p.x[i];
    return *this;
  }
  inline parameter &set(const parameter &p) {
    for (int i = 0; i < 7; ++i)
      x[i] = p.x[i];
  }
};

parameter defaultParameter(0.054099, 0.026865, 0.59269, 0.79123, 0.12353, 0.0076286, 0.060922);

inline parameter gaussian(const parameter &p) {
  srand((unsigned)time(NULL));
  default_random_engine generator;
  double delta[7] = {0};
  for (int i = 0; i < 7; ++i) {
    normal_distribution<double> distribution(p.x[i] / 3, p.x[i] / 12);
    if (rand() % 2 == 1)
      delta[i] = distribution(generator);
    else
      delta[i] = -distribution(generator);
  }
  parameter d(delta);
  return d;
}

inline parameter normalization(const parameter &p) {
  parameter tmp(p);
  double norm = 0;
  for (int i = 0; i < 7; ++i)
    norm += p.x[i] * p.x[i];
  norm = sqrt(norm);
  for (int i = 0; i < 7; ++i)
    tmp.x[i] /= norm;
  return tmp;
}

parameter currParameter(normalization(defaultParameter));

inline parameter operator+(const parameter &a, const parameter &b) {
  parameter res(defaultParameter);
  for (int i = 0; i < 7; ++i) {
    res.x[i] = a.x[i] + b.x[i];
  }
  return res;
}
inline parameter operator-(const parameter &a, const parameter &b) {
  parameter res(defaultParameter);
  for (int i = 0; i < 7; ++i) {
    res.x[i] = a.x[i] - b.x[i];
  }
  return res;
}
ostream &operator<<(ostream &out, parameter &p) {
  out << setprecision(5) << p.x[0] << ' ' << p.x[1] << ' ' << p.x[2] << ' '
      << p.x[3] << ' ' << p.x[4] << ' ' << p.x[5] << ' ' << p.x[6];
  return out;
}
inline parameter operator*(const double eta, const parameter &p) {
  parameter res(p);
  for (int i = 0; i < 7; ++i)
    res.x[i] *= eta;
  return res;
}

struct Decision {
  int x, y, o, t;
  Decision(int x_, int y_, int o_, int t_) : x(x_), y(y_), o(o_), t(t_) {}
  inline Decision &set(int x_, int y_, int o_, int t_) {
    x = x_;
    y = y_;
    o = o_;
    t = t_;
    return *this;
  }
  inline Decision &operator=(const Decision &src) {
    x = src.x;
    y = src.y;
    o = src.o;
    t = src.t;
    return *this;
  }
};

istream &operator>>(istream &in, Decision &d) {
  in >> d.t >> d.x >> d.y >> d.o;
  return in;
}

ostream &operator<<(ostream &out, Decision &d) {
  out << d.t << ' ' << d.x << ' ' << d.y << ' ' << d.o;
  return out;
}

Decision currDecision(0, 0, 0, 0);

class Node {
  // 复制粘贴的小麻雀
public:
  int transCount_[2], maxHeight_[2];
  int trans_[2][4][MAPWIDTH + 2];
  int elimTotal_[2];
  int lineTotal_[2];
  int gridInfo_[2][MAPHEIGHT + 2][MAPWIDTH + 2]; // 两队的地图
  int typeCountForColor_[2][7];                  // 两队的方块数
  int nextTypeForColor_[2];                      // 两队的当前方块
  int dead; // 当前死亡的玩家编号，-1表示没有玩家死亡

  Node(const Node &currNode, const Decision &player1, const Decision &player2) {
    memset(transCount_, 0, sizeof(transCount_));
    memset(maxHeight_, 0, sizeof(maxHeight_));
    memset(trans_, 0, sizeof(trans_));
    memcpy(elimTotal_, currNode.elimTotal_, sizeof(elimTotal_));
    memcpy(lineTotal_, currNode.lineTotal_, sizeof(lineTotal_));
    memcpy(gridInfo_, currNode.gridInfo_, sizeof(gridInfo_));
    memcpy(nextTypeForColor_, currNode.nextTypeForColor_,
           sizeof(nextTypeForColor_));
    memcpy(typeCountForColor_, currNode.typeCountForColor_,
           sizeof(typeCountForColor_));
    Tetris myTetris(nextTypeForColor_[currBotColor], currBotColor);
    myTetris.set(player1.x, player1.y, player1.o);
    if (!place(myTetris))
      cerr << "what?" << endl;
    Tetris enemyTetris(nextTypeForColor_[enemyColor], enemyColor);
    enemyTetris.set(player2.x, player2.y, player2.o);
    if (!place(enemyTetris))
      cerr << "what?" << endl;
    nextTypeForColor_[currBotColor] = player2.t;
    nextTypeForColor_[enemyColor] = player1.t;
    ++typeCountForColor_[currBotColor][player2.t];
    ++typeCountForColor_[enemyColor][player1.t];
    eliminate(0);
    eliminate(1);
    dead = transfer();
  }

  Node(int initType) {
    int i;
    memset(gridInfo_, 0, sizeof(gridInfo_));
    for (i = 0; i < MAPHEIGHT + 2; ++i) {
      gridInfo_[1][i][0] = gridInfo_[1][i][MAPWIDTH + 1] = -2;
      gridInfo_[0][i][0] = gridInfo_[0][i][MAPWIDTH + 1] = -2;
    }
    for (i = 0; i < MAPWIDTH + 2; ++i) {
      gridInfo_[1][0][i] = gridInfo_[1][MAPHEIGHT + 1][i] = -2;
      gridInfo_[0][0][i] = gridInfo_[0][MAPHEIGHT + 1][i] = -2;
    }
    memset(transCount_, 0, sizeof(transCount_));
    memset(maxHeight_, 0, sizeof(maxHeight_));
    memset(trans_, 0, sizeof(trans_));
    memset(elimTotal_, 0, sizeof(elimTotal_));
    memset(lineTotal_, 0, sizeof(lineTotal_));
    memset(typeCountForColor_, 0, sizeof(typeCountForColor_));
    nextTypeForColor_[0] = nextTypeForColor_[1] = initType;
    ++typeCountForColor_[0][initType];
    ++typeCountForColor_[1][initType];
    dead = -1;
  }

  Node &operator=(const Node &tmp) {
    memcpy(transCount_, tmp.transCount_, sizeof(transCount_));
    memcpy(maxHeight_, tmp.maxHeight_, sizeof(maxHeight_));
    memcpy(trans_, tmp.trans_, sizeof(trans_));
    memcpy(elimTotal_, tmp.elimTotal_, sizeof(elimTotal_));
    memcpy(lineTotal_, tmp.lineTotal_, sizeof(lineTotal_));
    memcpy(gridInfo_, tmp.gridInfo_, sizeof(gridInfo_)); // 两队的地图
    memcpy(typeCountForColor_, tmp.typeCountForColor_,
           sizeof(typeCountForColor_)); // 两队的方块数
    memcpy(nextTypeForColor_, tmp.nextTypeForColor_,
           sizeof(nextTypeForColor_)); // 两队的当前方块
    dead = tmp.dead;
    return *this;
  }

  inline bool isValid(const Tetris &examTetris) {
    int x = examTetris.blockX, y = examTetris.blockY,
        o = examTetris.orientation;
    if (o < 0 || o > 3)
      return false;
    int i, tmpX, tmpY;
    for (i = 0; i < 4; ++i) {
      // cout << "haha!\n";
      tmpX = x + examTetris.shape[o][2 * i];
      tmpY = y + examTetris.shape[o][2 * i + 1];
      if (tmpX < 1 || tmpX > MAPWIDTH || tmpY < 1 || tmpY > MAPHEIGHT ||
          gridInfo_[examTetris.color][tmpY][tmpX] != 0)
        return false;
    }
    return true;
  }

  inline bool onGround(const Tetris &examTetris) {
    Tetris nextTetris(examTetris);
    --nextTetris.blockY;
    if (isValid(examTetris) && !isValid(nextTetris))
      return true;
    return false;
  }

  // 将方块放置在场地上
  inline bool place(const Tetris &examTetris) {
    if (!onGround(examTetris))
      return false;

    int i, tmpX, tmpY;
    for (i = 0; i < 4; ++i) {
      tmpX =
          examTetris.blockX + examTetris.shape[examTetris.orientation][2 * i];
      tmpY = examTetris.blockY +
             examTetris.shape[examTetris.orientation][2 * i + 1];
      gridInfo_[examTetris.color][tmpY][tmpX] = 2;
    }
    return true;
  }

  // 检查能否从场地顶端直接落到当前位置
  inline bool checkDirectDropTo(int color, int blockType, int x, int y, int o) {
    const int *def = blockShape[blockType][o];
    for (; y <= MAPHEIGHT; ++y)
      for (int i = 0; i < 4; ++i) {
        int _x = def[i * 2] + x, _y = def[i * 2 + 1] + y;
        if (_y > MAPHEIGHT)
          continue;
        if (_y < 1 || _x < 1 || _x > MAPWIDTH || gridInfo_[color][_y][_x])
          return false;
      }
    return true;
  }

  // 消去行
  void eliminate(int color) {
    int &count = transCount_[color] = 0;
    int i, j, emptyFlag, fullFlag;
    maxHeight_[color] = MAPHEIGHT;
    for (i = 1; i <= MAPHEIGHT; ++i) {
      emptyFlag = 1;
      fullFlag = 1;
      for (j = 1; j <= MAPWIDTH; ++j) {
        if (gridInfo_[color][i][j] == 0)
          fullFlag = 0;
        else
          emptyFlag = 0;
      }
      if (fullFlag) {
        for (j = 1; j <= MAPWIDTH; ++j) {
          // 注意这里只转移以前的块，不包括最后一次落下的块（“撤销最后一步”）
          trans_[color][count][j] = gridInfo_[color][i][j] == 1 ? 1 : 0;
          gridInfo_[color][i][j] = 0;
        }
        count++;
      } else if (emptyFlag) {
        maxHeight_[color] = i - 1;
        break;
      } else
        for (j = 1; j <= MAPWIDTH; ++j) {
          gridInfo_[color][i - count][j] =
              gridInfo_[color][i][j] > 0 ? 1 : gridInfo_[color][i][j];
          if (count)
            gridInfo_[color][i][j] = 0;
        }
    }
    maxHeight_[color] -= count;
    elimTotal_[color] += elimBonus[count];
    lineTotal_[color] += count;
    // cout << lineTotal_[color] << endl;
  }

  // 转移双方消去的行，返回-1表示继续，否则返回输者
  int transfer() {
    int color1 = 0, color2 = 1;
    if (transCount_[color1] == 0 && transCount_[color2] == 0)
      return -1;
    if (transCount_[color1] == 0 || transCount_[color2] == 0) {
      if (transCount_[color1] == 0 && transCount_[color2] > 0)
        swap(color1, color2);
      int h2;
      maxHeight_[color2] = h2 = maxHeight_[color2] + transCount_[color1];
      if (h2 > MAPHEIGHT)
        return color2;
      int i, j;

      for (i = h2; i > transCount_[color1]; --i)
        for (j = 1; j <= MAPWIDTH; ++j)
          gridInfo_[color2][i][j] =
              gridInfo_[color2][i - transCount_[color1]][j];

      for (i = transCount_[color1]; i > 0; --i)
        for (j = 1; j <= MAPWIDTH; ++j)
          gridInfo_[color2][i][j] = trans_[color1][i - 1][j];
      return -1;
    } else {
      int h1, h2;
      maxHeight_[color1] = h1 =
          maxHeight_[color1] +
          transCount_[color2]; //从color1处移动count1去color2
      maxHeight_[color2] = h2 = maxHeight_[color2] + transCount_[color1];

      if (h1 > MAPHEIGHT)
        return color1;
      if (h2 > MAPHEIGHT)
        return color2;

      int i, j;
      for (i = h2; i > transCount_[color1]; --i)
        for (j = 1; j <= MAPWIDTH; ++j)
          gridInfo_[color2][i][j] =
              gridInfo_[color2][i - transCount_[color1]][j];

      for (i = transCount_[color1]; i > 0; --i)
        for (j = 1; j <= MAPWIDTH; ++j)
          gridInfo_[color2][i][j] = trans_[color1][i - 1][j];

      for (i = h1; i > transCount_[color2]; --i)
        for (j = 1; j <= MAPWIDTH; ++j)
          gridInfo_[color1][i][j] =
              gridInfo_[color1][i - transCount_[color2]][j];

      for (i = transCount_[color2]; i > 0; --i)
        for (j = 1; j <= MAPWIDTH; ++j)
          gridInfo_[color1][i][j] = trans_[color2][i - 1][j];

      return -1;
    }
  }

  // 打印场地用于调试
  inline void printField() {
#ifndef _BOTZONE_ONLINE
    static const char *i2s[] = {"~~", "~~", "  ", "[]", "##"};
    cout << "~~：墙，[]：块，##：新块" << endl;
    for (int y = MAPHEIGHT + 1; y >= 0; --y) {
      for (int x = 0; x <= MAPWIDTH + 1; ++x)
        cout << i2s[gridInfo_[0][y][x] + 2];
      for (int x = 0; x <= MAPWIDTH + 1; ++x)
        cout << i2s[gridInfo_[1][y][x] + 2];
      cout << endl;
    }
#endif
  }
};

class SingleNode {
public:
  int elimRows;
  bool dead;          //有没有死
  bool position_step; // = 1，下一步该找位置；= 0, 下一步该给方块
  int gridInfo_[MAPHEIGHT + 2][MAPWIDTH + 2]; //这个队伍的地图
  int typeCountForColor_[7];                  //方块数
  int nextTypeForColor_; //当前方块， position step = 1 时它有意义
  list<position> availablePosition; // position step = 1 时它有意义
  list<int> availableTetris;        // position step = 0 时它有意义
  Tetris lastTetris;                //上一次放的方块
  int cheat_table[MAPHEIGHT + 2][MAPWIDTH + 2][4];

  SingleNode(const Node &currNode, bool isMine) : lastTetris(-1, -1) {
    elimRows = 0;
    int color = isMine ? currBotColor : enemyColor;
    position_step = true;
    memcpy(gridInfo_, currNode.gridInfo_[color], sizeof(gridInfo_));
    // printField();
    memcpy(typeCountForColor_, currNode.typeCountForColor_[color],
           sizeof(typeCountForColor_));
    nextTypeForColor_ = currNode.nextTypeForColor_[color];
    Tetris tmp_tetris(nextTypeForColor_, -1);
    memset(cheat_table, 0, sizeof(cheat_table));
    color_cheat_table(nextTypeForColor_);
    availablePosition.clear();
    for (int y = 1; y <= MAPHEIGHT; ++y) {
      for (int x = 1; x <= MAPWIDTH; ++x) {
        for (int o = 0; o < 4; ++o) {
          tmp_tetris.set(x, y, o);
          if (onGround(tmp_tetris)) {
            if (cheat_table[y][x][o] == 2) {
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

  SingleNode(const SingleNode &currSingleNode, const position &place_position)
      : lastTetris(currSingleNode.nextTypeForColor_, -1) {
    lastTetris.set(place_position.x, place_position.y, place_position.o);
    dead = currSingleNode.dead;
    elimRows = currSingleNode.elimRows;
    position_step = false;
    memcpy(gridInfo_, currSingleNode.gridInfo_, sizeof(gridInfo_));
    memcpy(typeCountForColor_, currSingleNode.typeCountForColor_,
           sizeof(typeCountForColor_));
    nextTypeForColor_ = -1;
    availablePosition.clear();
    availableTetris.clear();
    copy(currSingleNode.availableTetris.begin(),
         currSingleNode.availableTetris.end(), back_inserter(availableTetris));
    place(lastTetris);
    eliminate();
    // printField();
  } // 从状态 1 给出 position 向状态 0 转化
  SingleNode(const SingleNode &currSingleNode, const int give_colortype)
      : lastTetris(currSingleNode.lastTetris) {
    elimRows = currSingleNode.elimRows;
    position_step = true;
    memcpy(gridInfo_, currSingleNode.gridInfo_, sizeof(gridInfo_));
    memcpy(typeCountForColor_, currSingleNode.typeCountForColor_,
           sizeof(typeCountForColor_));
    ++typeCountForColor_[give_colortype];
    nextTypeForColor_ = give_colortype;
    memset(cheat_table, 0, sizeof(cheat_table));
    availablePosition.clear();
    Tetris tmp_tetris(give_colortype, -1);
    color_cheat_table(give_colortype);
    for (int y = 1; y <= MAPHEIGHT; ++y) {
      for (int x = 1; x <= MAPWIDTH; ++x) {
        for (int o = 0; o < 4; ++o) {
          tmp_tetris.set(x, y, o);
          if (onGround(tmp_tetris)) {
            if (cheat_table[y][x][o] == 2) {
              position p = {x, y, o};
              availablePosition.push_back(p);
            }
          }
        }
      }
    }
    dead = availablePosition.empty();
    findInvalidNext();
  } // 从状态 0 给出 colortype 向状态 1 转化

  inline bool isValid(const Tetris &examTetris) {
    int x = examTetris.blockX, y = examTetris.blockY,
        o = examTetris.orientation;
    if (o < 0 || o > 3)
      return false;
    int i, tmpX, tmpY;
    for (i = 0; i < 4; ++i) {
      tmpX = x + examTetris.shape[o][2 * i];
      tmpY = y + examTetris.shape[o][2 * i + 1];
      if (tmpX < 1 || tmpX > MAPWIDTH || tmpY < 1 || tmpY > MAPHEIGHT ||
          gridInfo_[tmpY][tmpX] != 0)
        return false;
    }
    return true;
  }

  inline void printField() {
#ifndef _BOTZONE_ONLINE
    static const char *i2s[] = {"~~", "~~", "  ", "[]", "##"};
    cout << "~~：墙，[]：块，##：新块" << endl;
    for (int y = MAPHEIGHT + 1; y >= 0; --y) {
      for (int x = 0; x <= MAPWIDTH + 1; ++x)
        cout << i2s[gridInfo_[y][x] + 2];
      cout << endl;
    }
#endif
  }

  inline bool onGround(const Tetris &examTetris) {
    Tetris nextTetris(examTetris);
    --nextTetris.blockY;
    if (isValid(examTetris) && !isValid(nextTetris))
      return true;
    return false;
  }

  inline bool checkDirectDropTo(int blockType, int x, int y, int o) {
    const int *def = blockShape[blockType][o];
    for (; y <= MAPHEIGHT; ++y)
      for (int i = 0; i < 4; ++i) {
        int _x = def[i * 2] + x, _y = def[i * 2 + 1] + y;
        if (_y > MAPHEIGHT)
          continue;
        if (_y < 1 || _x < 1 || _x > MAPWIDTH || gridInfo_[_y][_x])
          return false;
      }
    return true;
  }

  inline bool place(const Tetris &examTetris) {
    if (!onGround(examTetris))
      return false;

    int i, tmpX, tmpY;
    for (i = 0; i < 4; ++i) {
      tmpX =
          examTetris.blockX + examTetris.shape[examTetris.orientation][2 * i];
      tmpY = examTetris.blockY +
             examTetris.shape[examTetris.orientation][2 * i + 1];
      gridInfo_[tmpY][tmpX] = 2;
    }
    return true;
  }
  inline void eliminate() {
    int count = 0;
    int i, j, emptyFlag, fullFlag;
    int maxHeight_ = MAPHEIGHT;
    for (i = 1; i <= MAPHEIGHT; ++i) {
      emptyFlag = 1;
      fullFlag = 1;
      for (j = 1; j <= MAPWIDTH; ++j) {
        if (gridInfo_[i][j] == 0)
          fullFlag = 0;
        else
          emptyFlag = 0;
      }
      if (fullFlag) {
        for (j = 1; j <= MAPWIDTH; ++j)
          gridInfo_[i][j] = 0;
        ++count;
      } else if (emptyFlag) {
        maxHeight_ = i - 1;
        break;
      } else
        for (j = 1; j <= MAPWIDTH; ++j) {
          gridInfo_[i - count][j] = gridInfo_[i][j] > 0 ? 1 : gridInfo_[i][j];
          if (count)
            gridInfo_[i][j] = 0;
        }
    }
    maxHeight_ -= count;
    elimRows += count;
  }

  inline void color_the_neighbor(int blockType, int x, int y, int o) {
    if (cheat_table[y][x][o] != 0)
      return;
    Tetris tmp_tetris(blockType, -1);
    tmp_tetris.set(x, y, o);
    if (!isValid(tmp_tetris)) {
      cheat_table[y][x][o] = 1;
      return;
    }
    cheat_table[y][x][o] = 2;
    color_the_neighbor(blockType, x - 1, y, o);
    color_the_neighbor(blockType, x + 1, y, o);
    color_the_neighbor(blockType, x, y - 1, o);
    color_the_neighbor(blockType, x, y, (o + 1) % 4);
    return;
  }

  inline void color_cheat_table(int blockType) {
    for (int y = MAPHEIGHT; y > 0; --y) {
      for (int x = 1; x <= MAPWIDTH; ++x)
        for (int o = 0; o < 4; ++o) {
          if (cheat_table[y][x][o] != 0)
            continue;
          if (checkDirectDropTo(blockType, x, y, o)) {
            color_the_neighbor(blockType, x, y, o);
          }
        }
    }
  }

  inline void findInvalidNext() {
    availableTetris.clear();
    int maxCount = 0, minCount = 99;
    for (int i = 0; i < 7; ++i) {
      if (typeCountForColor_[i] > maxCount)
        maxCount = typeCountForColor_[i];
      if (typeCountForColor_[i] < minCount)
        minCount = typeCountForColor_[i];
    }
    if (maxCount - minCount >= 2) {
      for (int t = 0; t < 7; ++t) {
        if (typeCountForColor_[t] != maxCount)
          availableTetris.push_back(t);
      }
    } else
      for (int t = 0; t < 7; ++t)
        availableTetris.push_back(t);
  }

  inline double eval() {
    /*int &f1LowestHeightOfPosition = f[0], &f2HighestHeightOfPosition = f[1],
        &f4NumberofHoles = f[2], &f5NumberofLinesCompleted = f[3],
        &f6SurfaceRoughness = f[4], &f9MAXheight = f[5], &f10TotalHeight =
       f[6];*/
    int f0 = lastTetris.blockY +
             LowestHeight[lastTetris.orientation][lastTetris.blockType];
    int f1 = lastTetris.blockY +
             HighstHeight[lastTetris.orientation][lastTetris.blockType];
    int f3 = elimRows;
    int f2 = 0, f4 = 0, f5 = 0, f6 = 0;
    // double f3averageHeightOfPosition =
    //  lastTetris.blockY +
    // AverageHeight[lastTetris.orientation][lastTetris.blockType];
    int height_[MAPWIDTH + 2];
    for (int col = 1; col <= MAPWIDTH; ++col) {
      height_[col] = 0;
      for (int i = MAPHEIGHT; i >= 1; --i)
        if (gridInfo_[i][col] != 0) {
          height_[col] = i;
          break;
        }
    }
    for (int col = 1; col <= MAPWIDTH; ++col) {
      f6 += height_[col];
      if (height_[col] > f5)
        f5 = height_[col];
    }
    for (int col = 1; col <= MAPWIDTH; ++col)
      for (int i = height_[col]; i >= 1; --i)
        if (gridInfo_[i][col] == 0)
          ++f2;
    for (int col = 1; col <= MAPWIDTH - 1; ++col)
      f4 += abs(height_[col] - height_[col + 1]);

    double total__ = 0;
    total__ -= currParameter.x[0] * f0 + currParameter.x[1] * f1 +
               currParameter.x[2] * f2 - currParameter.x[3] * f3 +
               currParameter.x[4] * f4 + currParameter.x[6] * f6;
    if (f5 > 15) // 惩罚MAXheight
      total__ -= currParameter.x[5] * f5;
    return total__;
  }
};

double alphabeta_singlemap(SingleNode &currSingleNode_, int depth, double alpha,
                           double beta, bool is_me, int init_depth,
                           position *BestPosition) {
  if (depth == 0 || currSingleNode_.dead == 1) {
    double val = currSingleNode_.eval();
    return val;
  }
  if (is_me == 1) {
    double val = -Inf_double;
    for (list<position>::iterator itr =
             currSingleNode_.availablePosition.begin();
         itr != currSingleNode_.availablePosition.end(); ++itr) {
      SingleNode thisChild(currSingleNode_, *itr);
      double tmp = alphabeta_singlemap(thisChild, depth - 1, alpha, beta, 0,
                                       init_depth, BestPosition);
      if (tmp > val) {
        val = tmp;
        if (depth == init_depth) {
          BestPosition->x = itr->x;
          BestPosition->y = itr->y;
          BestPosition->o = itr->o;
        }
      }
      if (tmp > alpha) {
        alpha = tmp;
      }
      if (beta <= alpha)
        break;
    }
    return val;
  } else {
    double val = Inf_double;
    for (list<int>::iterator itr = currSingleNode_.availableTetris.begin();
         itr != currSingleNode_.availableTetris.end(); ++itr) {
      SingleNode thisChild(currSingleNode_, *itr);
      double tmp = alphabeta_singlemap(thisChild, depth - 1, alpha, beta, 1,
                                       init_depth, BestPosition);
      if (tmp < val) {
        val = tmp;
      }
      if (tmp < beta) {
        beta = tmp;
      }
      if (beta <= alpha)
        break;
    }
    return val;
  }
}

void makeMyDecision(const Node &currNode_, int init_depth1, int init_depth2,
                    const parameter &p = defaultParameter) {
  currParameter.set(p);
  SingleNode MycurrSingleNode(currNode_, 1);
  SingleNode HiscurrSingleNode(currNode_, 0);
  if (MycurrSingleNode.availablePosition.empty()) {
    // cerr << "Bot " << currBotColor << ": I guess I am dead!" << endl;
    currDecision.set(-1, -1, -1, -1);
    return;
  }
  position MyBestPosition = MycurrSingleNode.availablePosition.front();
  int HisWorstTeris;
  alphabeta_singlemap(MycurrSingleNode, init_depth1, -Inf_double, Inf_double, 1,
                      init_depth1, &MyBestPosition);
  double enemymax[7];
  for (int i = 0; i < 7; ++i)
    enemymax[i] = -Inf_double;
  double minmax_ = Inf_double;

  for (list<position>::iterator itr =
           HiscurrSingleNode.availablePosition.begin();
       itr != HiscurrSingleNode.availablePosition.end(); ++itr) {
    SingleNode H1(HiscurrSingleNode, *itr);
    for (list<int>::iterator itr2 = H1.availableTetris.begin();
         itr2 != H1.availableTetris.end(); ++itr2) {
      SingleNode H2(H1, *itr2);
      position HisBestPosition = H2.availablePosition.front();
      double tmp = alphabeta_singlemap(H2, init_depth2, -Inf_double, Inf_double,
                                       1, init_depth2, &HisBestPosition);
      if (tmp > enemymax[*itr2])
        enemymax[*itr2] = tmp;
    }
  }
  for (list<int>::iterator itr = HiscurrSingleNode.availableTetris.begin();
       itr != HiscurrSingleNode.availableTetris.end(); ++itr) {
    if (enemymax[*itr] < minmax_) {
      minmax_ = enemymax[*itr];
      HisWorstTeris = *itr;
    }
  }

  currDecision.set(MyBestPosition.x, MyBestPosition.y, MyBestPosition.o,
                   HisWorstTeris);
#ifdef _BOTZONE_ONLINE
  cout << currDecision << endl;
#endif
}
