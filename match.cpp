#include "tetris.h"
#include <ctime>
#include <iostream>

using namespace std;

#define depth1 1
#define depth2 1

int N = 9;

int playGame(const parameter &p1, const parameter &p2) {
  // freopen("match.txt", "w", stdout);
  int scoreofBot0 = 0;
  srand((unsigned)time(NULL));
  int blockType, turns;
  int dead;
  Decision p1Decision(0, 0, 0, 0), p2Decision(0, 0, 0, 0);
  for (int i = 0; i < N; ++i) {
    cout << "Game " << i << "    ";
    blockType = rand() % 7;
    Node currNode(blockType);
    turns = 0;
    while (1) {
      dead = -1;
      ++turns;
      // currNode.printField();
      // cin.get();
      makeMyDecision(currNode, depth1, depth2, p1);
      p1Decision = currDecision;
      currBotColor = 1;
      enemyColor = 0;
      makeMyDecision(currNode, depth1, depth2, p2);
      p2Decision = currDecision;
      if (p1Decision.t == -1) {
        if (p2Decision.t != -1)
          dead = 0;
        else {
          if (currNode.elimTotal_[0] < currNode.elimTotal_[1])
            dead = 0;
          else if (currNode.elimTotal_[0] == currNode.elimTotal_[2]) {
            dead = 388;
            ++scoreofBot0;
          } else {
            dead = 1;
            ++scoreofBot0;
            ++scoreofBot0;
          }
        }
      } else if (p2Decision.t == -1) {
        dead = 1;
        ++scoreofBot0;
        ++scoreofBot0;
      }
      if (dead != -1)
        break;
      currBotColor = 0;
      enemyColor = 1;
      Node newNode(currNode, p1Decision, p2Decision);
      currNode = newNode;
    }
    cout << "Dead player: " << dead << endl;
  }
  return scoreofBot0;
}

const int H = 20;
int H_0 = H;
double eta = 0.01;

int main() {
  freopen("miaowu~.txt", "w", stderr);
  clock_t b, e;
  int res;
  parameter tmpParameter(currParameter);
  cerr << "currParameter:   " << currParameter << endl;
  while (H_0--) {
    cout << "Round " << H - H_0 << " begins!" << endl;
    parameter delta = gaussian(currParameter);
    cout << "currParameter:   " << currParameter << endl;
    //cout << "delta:           " << delta << endl;
    parameter p1 = normalization(currParameter + delta),
              p2 = normalization(currParameter - delta);
    //cout << "p1 =             " << p1 << endl
         //<< "p2 =             " << p2 << endl;
    tmpParameter = currParameter;
    b = clock();
    res = playGame(p1, p2); // note that currParameter has changed!
    e = clock();
    cout << "After time = " << (e - b) / 1000. << " seconds, the Round is over!"
         << endl;
    cout << "Result: " << res << ":" << 2 * N - res << endl;
    currParameter = tmpParameter + ((eta * (res - N)) * delta);
    //cout << "Abnormal:        " << currParameter << endl;
    currParameter = normalization(currParameter);
    cerr << "currParameter:   " << currParameter << endl;
    cout << endl;
  }
  cout << "Do I become stronger? Let's try to play against the original bot!"
       << endl;
  N *= 2;
  res = playGame(currParameter, normalization(defaultParameter));
  cout << "Result: Current Bot " << res << " vs Original Bot " << N * 2 - res
       << endl;
  cout << "Press enter any number to exit." << endl;
  cin >> res;
  return 0;
}
