#include "tetris.h"
#include <ctime>
#include <iostream>

using namespace std;

struct performance {
  double lines;
  double score;
  double tetrads;
};

#define depth1 4
#define depth2 0

const int N = 10;

performance playGame() {
  performance p = {0, 0, 0};
  // freopen("match.txt", "w", stdout);
  srand((unsigned)time(NULL));
  int blockType, turns;
  int dead;
  Decision p1Decision(0, 0, 0, 0), p2Decision(0, 0, 0, 0);
  for (int i = 0; i < N; ++i) {
    cout << "Game " << i << endl;
    blockType = rand() % 7;
    Node currNode(blockType);
    turns = 0;
    while (1) {
      dead = -1;
      ++turns;
      //currNode.printField();
      //cin.get();
      makeMyDecision(currNode, depth1, depth2);
      p1Decision = currDecision;
      if (p1Decision.t == -1) {
        dead = 0;
      }
      currBotColor = 1;
      enemyColor = 0;
      makeMyDecision(currNode, depth1, depth2);
      p2Decision = currDecision;
      if (p2Decision.t == -1 && (dead == -1 || currNode.elimTotal_[1] <= currNode.elimTotal_[0]))
        dead = 1;
      if (dead != -1)
        break;
      currBotColor = 0;
      enemyColor = 1;
      Node newNode(currNode, p1Decision, p2Decision);
      currNode = newNode;
    }
    double penalty;
    if (dead == 0)
      penalty = 0.8;
    else
      penalty = 1;
    p.lines += currNode.lineTotal_[0] * penalty;
    p.score += currNode.elimTotal_[0] * penalty;
    p.tetrads += turns * penalty;
    cout << currNode.lineTotal_[0] * penalty << ' ' << currNode.elimTotal_[0] * penalty << ' ' << turns * penalty<< ' ' << endl;
  }
  p.lines /= N;
  p.score /= N;
  p.tetrads /= N;
  cout << p.lines << ' ' << p.score << ' ' << p.tetrads << ' ' << endl;
  return p;
}

int main() {
  playGame();
  cin.get();
  return 0;
}