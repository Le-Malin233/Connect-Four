#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <climits>

using namespace std;
using namespace chrono;

const int ROWS = 6;
const int COLS = 7;
const int WIN_LEN = 4;
const int EMPTY = 0;
const int AI_PIECE = 1;      // AI 执黑（先手）
const int HUMAN_PIECE = -1;  // 人类执白（后手）

// 方向数组：水平、垂直、对角线（右下、左下）
const int DIRS[4][2] = { {0,1},{1,0},{1,1},{1,-1} };

// 评估函数中不同棋子数量的分数权重
const int SCORE_ONE = 1;
const int SCORE_TWO = 10;
const int SCORE_THREE = 100;
const int SCORE_WIN = 1000000;

class ConnectFour {
private:
    int board[ROWS][COLS];
    int turn;                  // 当前应该谁走：AI_PIECE 或 HUMAN_PIECE

public:
    ConnectFour() {
        reset();
    }

    void reset() {
        memset(board, 0, sizeof(board));
        turn = AI_PIECE;       // AI 先手
    }

    // 显示棋盘（1为AI，2为人类，便于阅读）
    void printBoard() const {
        cout << "\n  1 2 3 4 5 6 7\n";
        for (int i = 0; i < ROWS; ++i) {
            cout << i + 1 << " ";
            for (int j = 0; j < COLS; ++j) {
                if (board[i][j] == EMPTY) cout << ". ";
                else if (board[i][j] == AI_PIECE) cout << "X ";
                else cout << "O ";
            }
            cout << "\n";
        }
        cout << endl;
    }

    // 检查某一列是否还有空位
    bool isValidMove(int col) const {
        if (col < 0 || col >= COLS) return false;
        return board[0][col] == EMPTY;
    }

    // 在指定列落子，返回是否成功
    bool dropPiece(int col, int player) {
        if (!isValidMove(col)) return false;
        for (int row = ROWS - 1; row >= 0; --row) {
            if (board[row][col] == EMPTY) {
                board[row][col] = player;
                return true;
            }
        }
        return false;
    }

    // 取消落子（用于搜索回退）
    void undoDrop(int col) {
        for (int row = 0; row < ROWS; ++row) {
            if (board[row][col] != EMPTY) {
                board[row][col] = EMPTY;
                return;
            }
        }
    }

    // 从最后一次落子位置判断是否胜利（效率高）
    bool checkWin(int player) const {
        // 水平方向
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c <= COLS - WIN_LEN; ++c) {
                bool win = true;
                for (int i = 0; i < WIN_LEN; ++i) {
                    if (board[r][c + i] != player) { win = false; break; }
                }
                if (win) return true;
            }
        }
        // 垂直方向
        for (int c = 0; c < COLS; ++c) {
            for (int r = 0; r <= ROWS - WIN_LEN; ++r) {
                bool win = true;
                for (int i = 0; i < WIN_LEN; ++i) {
                    if (board[r + i][c] != player) { win = false; break; }
                }
                if (win) return true;
            }
        }
        // 对角线（右下）
        for (int r = 0; r <= ROWS - WIN_LEN; ++r) {
            for (int c = 0; c <= COLS - WIN_LEN; ++c) {
                bool win = true;
                for (int i = 0; i < WIN_LEN; ++i) {
                    if (board[r + i][c + i] != player) { win = false; break; }
                }
                if (win) return true;
            }
        }
        // 对角线（左下）
        for (int r = WIN_LEN - 1; r < ROWS; ++r) {
            for (int c = 0; c <= COLS - WIN_LEN; ++c) {
                bool win = true;
                for (int i = 0; i < WIN_LEN; ++i) {
                    if (board[r - i][c + i] != player) { win = false; break; }
                }
                if (win) return true;
            }
        }
        return false;
    }

    // 判断棋盘是否已满（平局）
    bool isDraw() const {
        for (int c = 0; c < COLS; ++c)
            if (board[0][c] == EMPTY) return false;
        return true;
    }

    // 游戏是否结束（胜利或平局）
    bool isGameOver() const {
        if (checkWin(AI_PIECE) || checkWin(HUMAN_PIECE)) return true;
        return isDraw();
    }

    // 获取当前玩家（轮到谁）
    int getTurn() const { return turn; }
    void setTurn(int p) { turn = p; }
    void switchTurn() { turn = -turn; }

    // 获取棋盘（评估函数需要）
    int getBoard(int row, int col) const { return board[row][col]; }

    // 评估函数（对当前棋盘打分，从AI角度）
    // 统计一个连续4格窗口的得分
    int evaluateWindow(const int window[WIN_LEN]) const {
        int aiCnt = 0, humanCnt = 0;
        for (int i = 0; i < WIN_LEN; ++i) {
            if (window[i] == AI_PIECE) aiCnt++;
            else if (window[i] == HUMAN_PIECE) humanCnt++;
        }
        // 既有AI又有人类，无潜力
        if (aiCnt > 0 && humanCnt > 0) return 0;
        if (aiCnt == 0 && humanCnt == 0) return 0;
        if (aiCnt > 0) {
            if (aiCnt == 1) return SCORE_ONE;
            if (aiCnt == 2) return SCORE_TWO;
            if (aiCnt == 3) return SCORE_THREE;
            if (aiCnt == 4) return SCORE_WIN;
        }
        else if (humanCnt > 0) {
            if (humanCnt == 1) return -SCORE_ONE;
            if (humanCnt == 2) return -SCORE_TWO;
            if (humanCnt == 3) return -SCORE_THREE;
            if (humanCnt == 4) return -SCORE_WIN;
        }
        return 0;
    }

    // 全盘评估分数（AI视角）
    int evaluateBoard() const {
        int totalScore = 0;
        // 水平方向
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c <= COLS - WIN_LEN; ++c) {
                int window[WIN_LEN];
                for (int i = 0; i < WIN_LEN; ++i)
                    window[i] = board[r][c + i];
                totalScore += evaluateWindow(window);
            }
        }
        // 垂直方向
        for (int c = 0; c < COLS; ++c) {
            for (int r = 0; r <= ROWS - WIN_LEN; ++r) {
                int window[WIN_LEN];
                for (int i = 0; i < WIN_LEN; ++i)
                    window[i] = board[r + i][c];
                totalScore += evaluateWindow(window);
            }
        }
        // 对角线（右下）
        for (int r = 0; r <= ROWS - WIN_LEN; ++r) {
            for (int c = 0; c <= COLS - WIN_LEN; ++c) {
                int window[WIN_LEN];
                for (int i = 0; i < WIN_LEN; ++i)
                    window[i] = board[r + i][c + i];
                totalScore += evaluateWindow(window);
            }
        }
        // 对角线（左下）
        for (int r = WIN_LEN - 1; r < ROWS; ++r) {
            for (int c = 0; c <= COLS - WIN_LEN; ++c) {
                int window[WIN_LEN];
                for (int i = 0; i < WIN_LEN; ++i)
                    window[i] = board[r - i][c + i];
                totalScore += evaluateWindow(window);
            }
        }
        return totalScore;
    }

    // ---------- Minimax + Alpha-Beta 剪枝 ----------
    // depth: 剩余深度, alpha, beta, maximizingPlayer: 当前是否为AI
    // 返回值：当前局面的评估分数
    int minimax(int depth, int alpha, int beta, bool maximizingPlayer) {
        // 终止条件：达到深度 或 游戏结束
        if (depth == 0 || isGameOver()) {
            return evaluateBoard();
        }

        if (maximizingPlayer) {
            int maxEval = INT_MIN;
            for (int col = 0; col < COLS; ++col) {
                if (isValidMove(col)) {
                    dropPiece(col, AI_PIECE);
                    int eval = minimax(depth - 1, alpha, beta, false);
                    undoDrop(col);
                    maxEval = max(maxEval, eval);
                    alpha = max(alpha, eval);
                    if (beta <= alpha) break; // Beta剪枝
                }
            }
            return maxEval;
        }
        else {
            int minEval = INT_MAX;
            for (int col = 0; col < COLS; ++col) {
                if (isValidMove(col)) {
                    dropPiece(col, HUMAN_PIECE);
                    int eval = minimax(depth - 1, alpha, beta, true);
                    undoDrop(col);
                    minEval = min(minEval, eval);
                    beta = min(beta, eval);
                    if (beta <= alpha) break; // Alpha剪枝
                }
            }
            return minEval;
        }
    }

    // AI 选择最佳落子列（使用Alpha-Beta）
    int getBestMove(int depth) {
        int bestScore = INT_MIN;
        int bestCol = -1;
        // 尝试每一列，评估后选择最高分
        for (int col = 0; col < COLS; ++col) {
            if (isValidMove(col)) {
                dropPiece(col, AI_PIECE);
                int score = minimax(depth - 1, INT_MIN, INT_MAX, false);
                undoDrop(col);
                if (score > bestScore) {
                    bestScore = score;
                    bestCol = col;
                }
            }
        }
        return bestCol;
    }

    // 人类玩家落子（交互）
    bool humanMove() {
        int col;
        while (true) {
            cout << "请输入列号 (1-7): ";
            cin >> col;
            col--;
            if (isValidMove(col)) {
                dropPiece(col, HUMAN_PIECE);
                return true;
            }
            else {
                cout << "无效落子，该列已满！请重新选择。\n";
            }
        }
    }
};

// main函数：人机对战，并统计AI决策效率
int main() {
    ConnectFour game;
    int depth = 5;   // 搜索深度
    cout << "========== 四子棋 (AI vs 人类) ==========\n";
    cout << "AI 执 X (先手)，您执 O (后手)\n";
    cout << "AI 搜索深度 = " << depth << endl;
    game.printBoard();

    while (!game.isGameOver()) {
        if (game.getTurn() == AI_PIECE) {
            cout << "\nAI 思考中 ...\n";
            auto start = high_resolution_clock::now();
            int move = game.getBestMove(depth);
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start).count();
            if (move == -1) {
                cout << "AI 无合法落子，平局！\n";
                break;
            }
            game.dropPiece(move, AI_PIECE);
            cout << "AI 选择了第 " << move + 1 << " 列 (耗时 " << duration << " ms)\n";
            game.printBoard();
            if (game.checkWin(AI_PIECE)) {
                cout << "AI 胜利！\n";
                break;
            }
            game.switchTurn();
        }
        else {
            // 人类回合
            game.humanMove();
            game.printBoard();
            if (game.checkWin(HUMAN_PIECE)) {
                cout << "恭喜，您赢了！\n";
                break;
            }
            game.switchTurn();
        }
        if (game.isDraw()) {
            cout << "平局！棋盘已满。\n";
            break;
        }
    }
    cout << "游戏结束。\n";
    return 0;
}