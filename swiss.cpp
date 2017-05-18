#include "tetris.h"
#include <ctime>
#include <random>
#include <vector>

#define depth1 1
#define depth2 1

using namespace std;

const double weightoftb = 0.01;

class Bot {
public:
  parameter p;   // 参数
  int score, tb; // 场分和对手分
  Bot(parameter &p_ = defaultParameter, int score_ = 0, int tb_ = 0)
      : p(p_), score(score_), tb(tb_) {}
  Bot(const Bot &src) : p(src.p), score(src.score), tb(src.tb){};
  Bot(const Bot &parent1, const Bot &parent2) : p(parent1.p) { // 遗传
    double fitness1 = parent1.score + weightoftb * parent1.tb,
           fitness2 = parent2.score + weightoftb * parent2.tb,
           t1 = fitness1 / (fitness1 + fitness2),
           t2 = fitness2 / (fitness1 + fitness2);
    p = t1 * parent1.p + t2 * parent2.p;
    score = tb = 0;
  }
};

bool cmp(const Bot &b1, const Bot &b2) {
  if (b1.score < b2.score)
    return true;
  else if (b1.score == b2.score)
    return b1.tb < b2.tb;
  else
    return false;
}

ostream &operator<<(ostream &out, Bot &b) {
  out << "parameter: " << b.p << "score: " << b.score << "tb: " << b.tb;
  return out;
}

// two players playing the game

void playGame(Bot &player1,
              Bot &player2) { // currParameter 会被改为 player2 的参数
  int scoreofBot0 = 0, blockType, dead;
  const int battle = 7; // 无随机性
  Decision p1Decision(0, 0, 0, 0), p2Decision(0, 0, 0, 0);
  for (int i = 0; i < battle; ++i) {
    blockType = i; // 初值
    Node currNode(blockType);
    while (1) {
      dead = -1;
      makeMyDecision(currNode, depth1, depth2,
                     player1.p); // currParameter 被改为 p1
      p1Decision = currDecision;
      currBotColor = 1;
      enemyColor = 0;
      makeMyDecision(currNode, depth1, depth2,
                     player2.p); // currParameter 被改为 p2
      p2Decision = currDecision;
      if (p1Decision.t == -1) {
        if (p2Decision.t != -1)
          dead = 0;
        else {
          if (currNode.elimTotal_[0] < currNode.elimTotal_[1])
            dead = 0;
          else if (currNode.elimTotal_[0] == currNode.elimTotal_[1]) {
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
  }
  player1.tb += player2.score;
  player2.tb += player1.score;
  player1.score += scoreofBot0;
  player2.score += 2 * battle - scoreofBot0;
}

const int numberOfBots = 1000;          // 种群数量
const double mutationRate = 0.05;       // 突变率
const int numberOfNextGeneration = 300; // 新生代的数量
const int rangeOfCommunication = 100;   // 发生基因交流的范围
vector<Bot> population; // Bot 种群，维护使之按 fitness 排序

void swissTournament(int R) { // currParameter will change
  // R-round Swiss Tournament, the number of players must be even
  for (vector<Bot>::iterator itr = population.begin(); itr != population.end();
       ++itr) {
    itr->score = 0;
    itr->tb = 0;
  } // reset the score of players
  int N = population.size() / 2;
  while (R--) {
    cout << R << " rounds remain" << endl;
    vector<Bot>::iterator itr1 = population.begin(), itr2 = itr1 + N;
    int N_0 = N;
    while (N_0--) {
      cout << "match " << N - N_0 << endl;
      playGame(*itr1, *itr2);
      ++itr1;
      ++itr2;
    }
    stable_sort(population.begin(), population.end(), cmp); // sort the players
  }
}

int numberofRounds = 10;

int main() {
  freopen("result.txt", "w", stderr);
  default_random_engine generator;
  normal_distribution<double> gaussDistribution(0., 1.);
  uniform_int_distribution<int> uniDistribution(0., numberOfBots - 1);

  // generate a number of parameters normalized to the unit sphere S^{LEN - 1}
  for (int i = 0; i < numberOfBots; ++i) {
    double x[LEN], norm = 0;
    for (int j = 0; j < LEN; ++j) {
      x[j] = abs(gaussDistribution(generator));
      norm += x[j] * x[j];
    }
    norm = sqrt(norm);
    for (int j = 0; j < LEN; ++j)
      x[j] /= sqrt(norm);
    parameter p(x);
    Bot b(p, 0.);
    population.push_back(b);
  }
  clock_t b, e;
  // initialize scores
  cout << "Initialization begins!" << endl;
  b = clock();
  swissTournament(numberofRounds);
  e = clock();
  cout << "After " << (e - b) / 1000. << " seconds, initialized!" << endl;
  int numberofGenerations = 1000;
  while (numberofGenerations--) {
    // generate the next generation
    for (int i = 0; i < numberOfNextGeneration; ++i) {
      int x1 = numberOfBots, x2 = numberOfBots, t;
      for (int j = 0; j < rangeOfCommunication; ++j) {
        t = uniDistribution(generator);
        if (t < x1) {
          x2 = x1;
          x1 = t;
        }
      }
      Bot son(population[x1], population[x2]);
      t = uniDistribution(generator);
      if (t < mutationRate * numberOfBots) { // 突变
        parameter delta = gaussian(son.p);
        son.p = son.p + delta;
      }
      son.p = normalization(son.p);
      population.push_back(son);
    }
    // delete bad bots
    b = clock();
    cout << "A tournament begins!" << endl;
    swissTournament(numberofRounds);
    population.resize(numberOfBots);
    e = clock();
    cout << "After " << (e - b) / 1000. << " seconds, the tournament ends!"
         << endl;
  }
  for (vector<Bot>::iterator itr = population.begin(); itr != population.end();
       ++itr) {
    cerr << *itr << endl;
  }
  return 0;
}