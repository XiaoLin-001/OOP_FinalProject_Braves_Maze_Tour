# Brave's Maze Tour（勇者探索迷宮）

四層迷宮探索遊戲。每一層要收集完所有鑰匙，再走到終點（Goal）才能進入下一層。

## 編譯與執行

### Linux（demo 環境）

```bash
make            # 編譯，產生 ./brave_tour
./brave_tour    # 開始遊戲
make clean      # 清除編譯產物
```

### Windows（本機開發，使用 MinGW）

`make` 需要安裝 mingw32-make，或直接用 g++ 編譯：

```powershell
g++ -std=c++11 -Wall -o brave_tour main.cpp Block.cpp Player.cpp Maze.cpp
.\brave_tour.exe
```

> 注意：地圖檔 `maze_1.txt ~ maze_4.txt` 需與執行檔放在**同一個資料夾**，
> 程式啟動時會從目前目錄讀取。

## 操作方式

| 按鍵 | 動作 |
|------|------|
| `W` | 向上移動 |
| `A` | 向左移動 |
| `S` | 向下移動 |
| `D` | 向右移動 |
| `E` | 離開遊戲 |

- 每按一次方向鍵後按 **Enter** 送出。
- 畫面會自動刷新，並在下方顯示樓層、鑰匙數量、攻擊力與遊戲時間。

## 遊戲畫面元素

| 圖案 | 意義 |
|------|------|
| `o` `/|\` | 勇者（移動時外觀會改變） |
| `###` | 牆壁，無法通過 |
| `?` | 尚未探索的格子（fog of war） |
| `$` | 終點 Goal（第 3、4 層會隨機移動） |
| `o / -|-` | 鑰匙 Key，撿起後消失 |
| `X` | 障礙物 Obstacle，需用攻擊力打破 |
| `@ O @` | 傳送門 Portal，成對出現 |

## 遊戲規則

1. 一開始只看得到勇者周圍 8 格，移動後會逐步揭開地圖，探索過的格子不會再變回未知。
2. 第 N 層有 N 把鑰匙，隨機散落在地圖上（每次載入位置都不同）。
3. 撞到障礙物時，每次扣 10 點 HP（障礙物 HP 為 10~50），歸零後消失才可通過。
4. 踩到傳送門會被送到另一個傳送門的位置。
5. 收集完該層所有鑰匙後走到終點，即可進入下一層；四層全破即獲勝。

## 程式架構

```
Block（基底：3x3 symbol、是否可見、type、player_touched）
 ├─ Empty / Wall / Goal      （助教提供的基本元素）
 │    └─ MovableGoal          （會移動的終點）
 ├─ Key                       （鑰匙）
 ├─ Obstacle                  （障礙物）
 └─ Portal                    （傳送門）

Player：座標、鑰匙數、攻擊力 ATK=10、動畫、計時
Maze  ：vector2d<Block*> 地圖、讀檔、隨機放置道具、渲染畫面
```

碰撞行為以多型的 `player_touched()` 實作：`Player::move()` 呼叫目標方塊的
`player_touched()`，由各方塊自己決定結果（撞牆、撿鑰匙、打障礙、傳送等）。
