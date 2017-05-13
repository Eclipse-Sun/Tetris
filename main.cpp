#include "tetris.h"
#include <iostream>

using namespace std;

int main() {
  freopen("aowu.txt", "r", stdin);
  freopen("miaowu.txt", "w", stdout);
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

  makeMyDecision(currNode, 4, 0);
  // The_direct_best(currNode);

  // cin.get();cin.get();
  return 0;
}
