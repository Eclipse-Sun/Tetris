#include "tetris.h"
#include <iostream>

using namespace std;

int main() {
#ifndef _BOTZONE_ONLINE
  freopen("aowu.txt", "r", stdin);
  freopen("miaowu.txt", "w", stdout);
#endif
  istream::sync_with_stdio(false);

  int turnID, blockType;
  cin >> turnID;

  cin >> blockType >> currBotColor;
  enemyColor = 1 - currBotColor;
  Node currNode(blockType);
  Decision player1(0, 0, 0, 0), player2(0, 0, 0, 0);
  for (int i = 1; i < turnID; ++i) {
    cin >> player1 >> player2;
    Node newNode(currNode, player1, player2);
    currNode = newNode;
  }

  // 遇事不决先输出（平台上编译不会输出）
  currNode.printField();
  clock_t b, e;
  b = clock();
  makeMyDecision(currNode, 4, 2);
  e = clock();
#ifndef _BOTZONE_ONLINE
  cout << currDecision << endl;
  cout << "Time: " << (e - b) / 1000. << endl;
#endif
  return 0;
}
